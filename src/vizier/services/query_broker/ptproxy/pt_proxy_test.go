package ptproxy_test

import (
	"context"
	"errors"
	"net"
	"testing"
	"time"

	"github.com/gogo/protobuf/proto"
	"github.com/gogo/protobuf/types"
	"github.com/nats-io/nats.go"
	"google.golang.org/grpc"
	"google.golang.org/grpc/codes"
	"google.golang.org/grpc/test/bufconn"

	"github.com/stretchr/testify/assert"
	"pixielabs.ai/pixielabs/src/shared/cvmsgspb"
	"pixielabs.ai/pixielabs/src/utils/testingutils"
	"pixielabs.ai/pixielabs/src/vizier/services/query_broker/ptproxy"
	vizierpb "pixielabs.ai/pixielabs/src/vizier/vizierpb"
)

const bufSize = 1024 * 1024

type MockVzServer struct {
	t *testing.T
}

func NewMockVzServer(t *testing.T) *MockVzServer {
	return &MockVzServer{t}
}

func (m *MockVzServer) ExecuteScript(req *vizierpb.ExecuteScriptRequest, srv vizierpb.VizierService_ExecuteScriptServer) error {
	if req.QueryStr == "should pass" {
		resp := &vizierpb.ExecuteScriptResponse{
			QueryID: "1",
		}
		srv.Send(resp)
	}
	if req.QueryStr == "error" {
		resp := &vizierpb.ExecuteScriptResponse{
			QueryID: "2",
		}
		srv.Send(resp)
		return errors.New("Failed")
	}
	if req.QueryStr == "cancel" {
		resp := &vizierpb.ExecuteScriptResponse{
			QueryID: "3",
		}
		srv.Send(resp)
		select {
		case <-time.After(2 * time.Second):
			return errors.New("Timeout waiting for context to be canceled")
		}
	}
	return nil
}

func (m *MockVzServer) HealthCheck(req *vizierpb.HealthCheckRequest, srv vizierpb.VizierService_HealthCheckServer) error {
	return nil
}

type testState struct {
	t        *testing.T
	lis      *bufconn.Listener
	vzServer *MockVzServer

	nc   *nats.Conn
	conn *grpc.ClientConn
}

func createTestState(t *testing.T) (*testState, func(t *testing.T)) {
	lis := bufconn.Listen(bufSize)
	s := grpc.NewServer()

	natsPort, natsCleanup := testingutils.StartNATS(t)
	nc, err := nats.Connect(testingutils.GetNATSURL(natsPort))
	if err != nil {
		t.Fatal(err)
	}

	vzServer := NewMockVzServer(t)
	vizierpb.RegisterVizierServiceServer(s, vzServer)

	go func() {
		if err := s.Serve(lis); err != nil {
			t.Fatalf("Server exited with error: %v\n", err)
		}
	}()

	ctx := context.Background()
	conn, err := grpc.DialContext(ctx, "bufnet", grpc.WithDialer(createDialer(lis)), grpc.WithInsecure())
	if err != nil {
		t.Fatalf("Failed to dial bufnet: %v", err)
	}

	cleanupFunc := func(t *testing.T) {
		natsCleanup()
		conn.Close()
	}

	return &testState{
		t:        t,
		lis:      lis,
		vzServer: vzServer,
		nc:       nc,
		conn:     conn,
	}, cleanupFunc
}

func createDialer(lis *bufconn.Listener) func(string, time.Duration) (net.Conn, error) {
	return func(str string, duration time.Duration) (conn net.Conn, e error) {
		return lis.Dial()
	}
}

func TestPassThroughProxy(t *testing.T) {
	tests := []struct {
		name          string
		requestID     string
		request       *vizierpb.ExecuteScriptRequest
		expectedResps []*cvmsgspb.V2CAPIStreamResponse
		sendCancel    bool
	}{
		{
			name:      "complete",
			requestID: "1",
			request: &vizierpb.ExecuteScriptRequest{
				QueryStr: "should pass",
			},
			expectedResps: []*cvmsgspb.V2CAPIStreamResponse{
				&cvmsgspb.V2CAPIStreamResponse{
					RequestID: "1",
					Msg: &cvmsgspb.V2CAPIStreamResponse_ExecResp{
						ExecResp: &vizierpb.ExecuteScriptResponse{
							QueryID: "1",
						},
					},
				},
				&cvmsgspb.V2CAPIStreamResponse{
					RequestID: "1",
					Msg: &cvmsgspb.V2CAPIStreamResponse_Status{
						Status: &vizierpb.Status{
							Code: int32(codes.OK),
						},
					},
				},
			},
		},
		{
			name:      "error",
			requestID: "2",
			request: &vizierpb.ExecuteScriptRequest{
				QueryStr: "error",
			},
			expectedResps: []*cvmsgspb.V2CAPIStreamResponse{
				&cvmsgspb.V2CAPIStreamResponse{
					RequestID: "2",
					Msg: &cvmsgspb.V2CAPIStreamResponse_ExecResp{
						ExecResp: &vizierpb.ExecuteScriptResponse{
							QueryID: "2",
						},
					},
				},
				&cvmsgspb.V2CAPIStreamResponse{
					RequestID: "2",
					Msg: &cvmsgspb.V2CAPIStreamResponse_Status{
						Status: &vizierpb.Status{
							Code: int32(codes.Internal),
						},
					},
				},
			},
		},
		{
			name:       "cancel",
			requestID:  "3",
			sendCancel: true,
			request: &vizierpb.ExecuteScriptRequest{
				QueryStr: "cancel",
			},
			expectedResps: []*cvmsgspb.V2CAPIStreamResponse{
				&cvmsgspb.V2CAPIStreamResponse{
					RequestID: "3",
					Msg: &cvmsgspb.V2CAPIStreamResponse_ExecResp{
						ExecResp: &vizierpb.ExecuteScriptResponse{
							QueryID: "3",
						},
					},
				},
			},
		},
	}

	for _, test := range tests {
		t.Run(test.name, func(t *testing.T) {
			ts, cleanup := createTestState(t)
			defer cleanup(t)

			client := vizierpb.NewVizierServiceClient(ts.conn)

			s, err := ptproxy.NewPassThroughProxy(ts.nc, client)
			assert.Nil(t, err)
			go s.Run()

			// Subscribe to reply channel.
			replyCh := make(chan *nats.Msg, 10)
			replySub, err := ts.nc.ChanSubscribe("v2c.reply-"+test.requestID, replyCh)
			defer replySub.Unsubscribe()

			// Publish execute script request.
			sr := &cvmsgspb.C2VAPIStreamRequest{
				RequestID: test.requestID,
				Token:     "abcd",
				Msg: &cvmsgspb.C2VAPIStreamRequest_ExecReq{
					ExecReq: test.request,
				},
			}
			reqAnyMsg, err := types.MarshalAny(sr)
			assert.Nil(t, err)
			c2vMsg := &cvmsgspb.C2VMessage{
				Msg: reqAnyMsg,
			}
			b, err := c2vMsg.Marshal()
			assert.Nil(t, err)

			err = ts.nc.Publish("c2v.VizierPassthroughRequest", b)
			assert.Nil(t, err)

			numResp := 0
			for numResp < len(test.expectedResps) {
				select {
				case msg := <-replyCh:
					v2cMsg := &cvmsgspb.V2CMessage{}
					err := proto.Unmarshal(msg.Data, v2cMsg)
					assert.Nil(t, err)
					resp := &cvmsgspb.V2CAPIStreamResponse{}
					err = types.UnmarshalAny(v2cMsg.Msg, resp)
					assert.Nil(t, err)
					assert.Equal(t, test.expectedResps[numResp], resp)
					numResp++
				case <-time.After(2 * time.Second):
					t.Fatal("Timed out")
				}
			}

			if !test.sendCancel {
				return
			}

			sr = &cvmsgspb.C2VAPIStreamRequest{
				RequestID: test.requestID,
				Token:     "abcd",
				Msg: &cvmsgspb.C2VAPIStreamRequest_CancelReq{
					CancelReq: &cvmsgspb.C2VAPIStreamCancel{},
				},
			}
			reqAnyMsg, err = types.MarshalAny(sr)
			assert.Nil(t, err)
			c2vMsg = &cvmsgspb.C2VMessage{
				Msg: reqAnyMsg,
			}
			b, err = c2vMsg.Marshal()
			assert.Nil(t, err)

			err = ts.nc.Publish("c2v.VizierPassthroughRequest", b)
			assert.Nil(t, err)

			cancelResp := &cvmsgspb.V2CAPIStreamResponse{
				RequestID: test.requestID,
				Msg: &cvmsgspb.V2CAPIStreamResponse_Status{
					Status: &vizierpb.Status{
						Code: int32(codes.Canceled),
					},
				},
			}

			select {
			case msg := <-replyCh:
				v2cMsg := &cvmsgspb.V2CMessage{}
				err := proto.Unmarshal(msg.Data, v2cMsg)
				assert.Nil(t, err)
				resp := &cvmsgspb.V2CAPIStreamResponse{}
				err = types.UnmarshalAny(v2cMsg.Msg, resp)
				assert.Nil(t, err)
				assert.Equal(t, cancelResp, resp)
			case <-time.After(2 * time.Second):
				t.Fatal("Timed out")
			}

		})
	}
}