// Code generated by MockGen. DO NOT EDIT.
// Source: service.pb.go

// Package mock_projectmanagerpb is a generated GoMock package.
package mock_projectmanagerpb

import (
	context "context"
	reflect "reflect"

	gomock "github.com/golang/mock/gomock"
	grpc "google.golang.org/grpc"
	uuidpb "px.dev/pixie/src/api/proto/uuidpb"
	projectmanagerpb "px.dev/pixie/src/cloud/project_manager/projectmanagerpb"
)

// MockProjectManagerServiceClient is a mock of ProjectManagerServiceClient interface.
type MockProjectManagerServiceClient struct {
	ctrl     *gomock.Controller
	recorder *MockProjectManagerServiceClientMockRecorder
}

// MockProjectManagerServiceClientMockRecorder is the mock recorder for MockProjectManagerServiceClient.
type MockProjectManagerServiceClientMockRecorder struct {
	mock *MockProjectManagerServiceClient
}

// NewMockProjectManagerServiceClient creates a new mock instance.
func NewMockProjectManagerServiceClient(ctrl *gomock.Controller) *MockProjectManagerServiceClient {
	mock := &MockProjectManagerServiceClient{ctrl: ctrl}
	mock.recorder = &MockProjectManagerServiceClientMockRecorder{mock}
	return mock
}

// EXPECT returns an object that allows the caller to indicate expected use.
func (m *MockProjectManagerServiceClient) EXPECT() *MockProjectManagerServiceClientMockRecorder {
	return m.recorder
}

// GetProjectByName mocks base method.
func (m *MockProjectManagerServiceClient) GetProjectByName(ctx context.Context, in *projectmanagerpb.GetProjectByNameRequest, opts ...grpc.CallOption) (*projectmanagerpb.ProjectInfo, error) {
	m.ctrl.T.Helper()
	varargs := []interface{}{ctx, in}
	for _, a := range opts {
		varargs = append(varargs, a)
	}
	ret := m.ctrl.Call(m, "GetProjectByName", varargs...)
	ret0, _ := ret[0].(*projectmanagerpb.ProjectInfo)
	ret1, _ := ret[1].(error)
	return ret0, ret1
}

// GetProjectByName indicates an expected call of GetProjectByName.
func (mr *MockProjectManagerServiceClientMockRecorder) GetProjectByName(ctx, in interface{}, opts ...interface{}) *gomock.Call {
	mr.mock.ctrl.T.Helper()
	varargs := append([]interface{}{ctx, in}, opts...)
	return mr.mock.ctrl.RecordCallWithMethodType(mr.mock, "GetProjectByName", reflect.TypeOf((*MockProjectManagerServiceClient)(nil).GetProjectByName), varargs...)
}

// GetProjectForOrg mocks base method.
func (m *MockProjectManagerServiceClient) GetProjectForOrg(ctx context.Context, in *uuidpb.UUID, opts ...grpc.CallOption) (*projectmanagerpb.ProjectInfo, error) {
	m.ctrl.T.Helper()
	varargs := []interface{}{ctx, in}
	for _, a := range opts {
		varargs = append(varargs, a)
	}
	ret := m.ctrl.Call(m, "GetProjectForOrg", varargs...)
	ret0, _ := ret[0].(*projectmanagerpb.ProjectInfo)
	ret1, _ := ret[1].(error)
	return ret0, ret1
}

// GetProjectForOrg indicates an expected call of GetProjectForOrg.
func (mr *MockProjectManagerServiceClientMockRecorder) GetProjectForOrg(ctx, in interface{}, opts ...interface{}) *gomock.Call {
	mr.mock.ctrl.T.Helper()
	varargs := append([]interface{}{ctx, in}, opts...)
	return mr.mock.ctrl.RecordCallWithMethodType(mr.mock, "GetProjectForOrg", reflect.TypeOf((*MockProjectManagerServiceClient)(nil).GetProjectForOrg), varargs...)
}

// IsProjectAvailable mocks base method.
func (m *MockProjectManagerServiceClient) IsProjectAvailable(ctx context.Context, in *projectmanagerpb.IsProjectAvailableRequest, opts ...grpc.CallOption) (*projectmanagerpb.IsProjectAvailableResponse, error) {
	m.ctrl.T.Helper()
	varargs := []interface{}{ctx, in}
	for _, a := range opts {
		varargs = append(varargs, a)
	}
	ret := m.ctrl.Call(m, "IsProjectAvailable", varargs...)
	ret0, _ := ret[0].(*projectmanagerpb.IsProjectAvailableResponse)
	ret1, _ := ret[1].(error)
	return ret0, ret1
}

// IsProjectAvailable indicates an expected call of IsProjectAvailable.
func (mr *MockProjectManagerServiceClientMockRecorder) IsProjectAvailable(ctx, in interface{}, opts ...interface{}) *gomock.Call {
	mr.mock.ctrl.T.Helper()
	varargs := append([]interface{}{ctx, in}, opts...)
	return mr.mock.ctrl.RecordCallWithMethodType(mr.mock, "IsProjectAvailable", reflect.TypeOf((*MockProjectManagerServiceClient)(nil).IsProjectAvailable), varargs...)
}

// RegisterProject mocks base method.
func (m *MockProjectManagerServiceClient) RegisterProject(ctx context.Context, in *projectmanagerpb.RegisterProjectRequest, opts ...grpc.CallOption) (*projectmanagerpb.RegisterProjectResponse, error) {
	m.ctrl.T.Helper()
	varargs := []interface{}{ctx, in}
	for _, a := range opts {
		varargs = append(varargs, a)
	}
	ret := m.ctrl.Call(m, "RegisterProject", varargs...)
	ret0, _ := ret[0].(*projectmanagerpb.RegisterProjectResponse)
	ret1, _ := ret[1].(error)
	return ret0, ret1
}

// RegisterProject indicates an expected call of RegisterProject.
func (mr *MockProjectManagerServiceClientMockRecorder) RegisterProject(ctx, in interface{}, opts ...interface{}) *gomock.Call {
	mr.mock.ctrl.T.Helper()
	varargs := append([]interface{}{ctx, in}, opts...)
	return mr.mock.ctrl.RecordCallWithMethodType(mr.mock, "RegisterProject", reflect.TypeOf((*MockProjectManagerServiceClient)(nil).RegisterProject), varargs...)
}

// MockProjectManagerServiceServer is a mock of ProjectManagerServiceServer interface.
type MockProjectManagerServiceServer struct {
	ctrl     *gomock.Controller
	recorder *MockProjectManagerServiceServerMockRecorder
}

// MockProjectManagerServiceServerMockRecorder is the mock recorder for MockProjectManagerServiceServer.
type MockProjectManagerServiceServerMockRecorder struct {
	mock *MockProjectManagerServiceServer
}

// NewMockProjectManagerServiceServer creates a new mock instance.
func NewMockProjectManagerServiceServer(ctrl *gomock.Controller) *MockProjectManagerServiceServer {
	mock := &MockProjectManagerServiceServer{ctrl: ctrl}
	mock.recorder = &MockProjectManagerServiceServerMockRecorder{mock}
	return mock
}

// EXPECT returns an object that allows the caller to indicate expected use.
func (m *MockProjectManagerServiceServer) EXPECT() *MockProjectManagerServiceServerMockRecorder {
	return m.recorder
}

// GetProjectByName mocks base method.
func (m *MockProjectManagerServiceServer) GetProjectByName(arg0 context.Context, arg1 *projectmanagerpb.GetProjectByNameRequest) (*projectmanagerpb.ProjectInfo, error) {
	m.ctrl.T.Helper()
	ret := m.ctrl.Call(m, "GetProjectByName", arg0, arg1)
	ret0, _ := ret[0].(*projectmanagerpb.ProjectInfo)
	ret1, _ := ret[1].(error)
	return ret0, ret1
}

// GetProjectByName indicates an expected call of GetProjectByName.
func (mr *MockProjectManagerServiceServerMockRecorder) GetProjectByName(arg0, arg1 interface{}) *gomock.Call {
	mr.mock.ctrl.T.Helper()
	return mr.mock.ctrl.RecordCallWithMethodType(mr.mock, "GetProjectByName", reflect.TypeOf((*MockProjectManagerServiceServer)(nil).GetProjectByName), arg0, arg1)
}

// GetProjectForOrg mocks base method.
func (m *MockProjectManagerServiceServer) GetProjectForOrg(arg0 context.Context, arg1 *uuidpb.UUID) (*projectmanagerpb.ProjectInfo, error) {
	m.ctrl.T.Helper()
	ret := m.ctrl.Call(m, "GetProjectForOrg", arg0, arg1)
	ret0, _ := ret[0].(*projectmanagerpb.ProjectInfo)
	ret1, _ := ret[1].(error)
	return ret0, ret1
}

// GetProjectForOrg indicates an expected call of GetProjectForOrg.
func (mr *MockProjectManagerServiceServerMockRecorder) GetProjectForOrg(arg0, arg1 interface{}) *gomock.Call {
	mr.mock.ctrl.T.Helper()
	return mr.mock.ctrl.RecordCallWithMethodType(mr.mock, "GetProjectForOrg", reflect.TypeOf((*MockProjectManagerServiceServer)(nil).GetProjectForOrg), arg0, arg1)
}

// IsProjectAvailable mocks base method.
func (m *MockProjectManagerServiceServer) IsProjectAvailable(arg0 context.Context, arg1 *projectmanagerpb.IsProjectAvailableRequest) (*projectmanagerpb.IsProjectAvailableResponse, error) {
	m.ctrl.T.Helper()
	ret := m.ctrl.Call(m, "IsProjectAvailable", arg0, arg1)
	ret0, _ := ret[0].(*projectmanagerpb.IsProjectAvailableResponse)
	ret1, _ := ret[1].(error)
	return ret0, ret1
}

// IsProjectAvailable indicates an expected call of IsProjectAvailable.
func (mr *MockProjectManagerServiceServerMockRecorder) IsProjectAvailable(arg0, arg1 interface{}) *gomock.Call {
	mr.mock.ctrl.T.Helper()
	return mr.mock.ctrl.RecordCallWithMethodType(mr.mock, "IsProjectAvailable", reflect.TypeOf((*MockProjectManagerServiceServer)(nil).IsProjectAvailable), arg0, arg1)
}

// RegisterProject mocks base method.
func (m *MockProjectManagerServiceServer) RegisterProject(arg0 context.Context, arg1 *projectmanagerpb.RegisterProjectRequest) (*projectmanagerpb.RegisterProjectResponse, error) {
	m.ctrl.T.Helper()
	ret := m.ctrl.Call(m, "RegisterProject", arg0, arg1)
	ret0, _ := ret[0].(*projectmanagerpb.RegisterProjectResponse)
	ret1, _ := ret[1].(error)
	return ret0, ret1
}

// RegisterProject indicates an expected call of RegisterProject.
func (mr *MockProjectManagerServiceServerMockRecorder) RegisterProject(arg0, arg1 interface{}) *gomock.Call {
	mr.mock.ctrl.T.Helper()
	return mr.mock.ctrl.RecordCallWithMethodType(mr.mock, "RegisterProject", reflect.TypeOf((*MockProjectManagerServiceServer)(nil).RegisterProject), arg0, arg1)
}
