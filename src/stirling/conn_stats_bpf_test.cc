#include "src/common/exec/subprocess.h"
#include "src/common/testing/testing.h"
#include "src/stirling/conn_stats_table.h"
#include "src/stirling/output.h"
#include "src/stirling/testing/client_server_system.h"
#include "src/stirling/testing/common.h"
#include "src/stirling/testing/socket_trace_bpf_test_fixture.h"

namespace pl {
namespace stirling {

using ::pl::stirling::testing::AccessRecordBatch;
using ::pl::stirling::testing::ClientServerSystem;
using ::pl::stirling::testing::FindRecordIdxMatchesPID;
using ::pl::stirling::testing::SendRecvScript;
using ::pl::stirling::testing::TCPSocket;
using ::testing::IsEmpty;
using ::testing::SizeIs;
using ::testing::UnorderedElementsAre;

class ConnStatsBPFTest : public testing::SocketTraceBPFTest</* TClientSideTracing */ true> {
 protected:
  DataTable data_table_{kConnStatsTable};
};

TEST_F(ConnStatsBPFTest, UnclassifiedEvents) {
  ClientServerSystem cs;

  SendRecvScript script;
  script.push_back({{"req1"}, {"resp1"}});
  script.push_back({{"req2"}, {"resp2"}});
  cs.RunClientServer<&TCPSocket::Read, &TCPSocket::Write>(script);

  {
    DataTable data_table(kHTTPTable);
    // Try to add 3 transfer iteration to go above the threshold.
    source_->TransferData(ctx_.get(), SocketTraceConnector::kHTTPTableNum, &data_table);
    source_->TransferData(ctx_.get(), SocketTraceConnector::kHTTPTableNum, &data_table);
    source_->TransferData(ctx_.get(), SocketTraceConnector::kHTTPTableNum, &data_table);
  }

  source_->TransferData(ctx_.get(), SocketTraceConnector::kConnStatsTableNum, &data_table_);
  std::vector<TaggedRecordBatch> tablets = data_table_.ConsumeRecords();
  ASSERT_FALSE(tablets.empty());
  types::ColumnWrapperRecordBatch record_batch = tablets[0].records;
  PrintRecordBatch("test", kConnStatsTable.ToProto(), record_batch);

  {
    auto indices = FindRecordIdxMatchesPID(record_batch, kPGSQLUPIDIdx, cs.ServerPID());
    ASSERT_THAT(indices, SizeIs(1));

    int conn_open =
        AccessRecordBatch<types::Int64Value>(record_batch, conn_stats_idx::kConnOpen, indices[0])
            .val;
    int conn_close =
        AccessRecordBatch<types::Int64Value>(record_batch, conn_stats_idx::kConnClose, indices[0])
            .val;
    int bytes_sent =
        AccessRecordBatch<types::Int64Value>(record_batch, conn_stats_idx::kBytesSent, indices[0])
            .val;
    int bytes_rcvd =
        AccessRecordBatch<types::Int64Value>(record_batch, conn_stats_idx::kBytesRecv, indices[0])
            .val;
    EXPECT_THAT(conn_open, 1);
    // TODO(yzhao): This should be 1. This fails because:
    // * SocketTraceConnector reads perf buffers in the order:
    //   "socket_control_events" "socket_data_events".
    // * conn_event_t is completely ignored, because its traffic_class cannot be resolved until
    // socket_data_event_t is received.
    // * When close_event_t is received, it is also ignored, as the conn_event_t is ignored,
    // and the socket_data_event_t has not arrived yet,
    EXPECT_THAT(conn_close, 0);
    EXPECT_THAT(bytes_sent, 10);
    EXPECT_THAT(bytes_rcvd, 8);
  }
  // Client process is not discovered by ConnectorContext (StandaloneContext) because it is
  // short-lived, and SocketTraceConnector does not export connection stats of processes that are
  // not reported, so client-side connection stats won't be exposed.
}

// Test fixture that starts SocketTraceConnector after the connection was already established.
class ConnStatsMidConnBPFTest : public testing::SocketTraceBPFTest</* TClientSideTracing */ true> {
 protected:
  void SetUp() override {
    server_.BindAndListen();

    // Server reads one message and exists.
    // The client will write one message in the test body, after the SocketTraceConnector is
    // created.
    server_thread_ = std::thread([this]() {
      auto socket = server_.Accept();
      std::string text;
      CHECK(socket->Read(&text));
      socket->Close();
    });

    client_.Connect(server_);

    // Now SocketTraceConnector is created after the connection was already established.
    SocketTraceBPFTest::SetUp();
  }

  void TearDown() override {
    if (server_thread_.joinable()) {
      server_thread_.join();
    }
    SocketTraceBPFTest::TearDown();
  }

  TCPSocket server_;
  std::thread server_thread_;

  TCPSocket client_;
  DataTable data_table_{kConnStatsTable};
};

TEST_F(ConnStatsMidConnBPFTest, DidNotSeeConnEstablishment) {
  std::string_view test_msg = "Hello World!";
  EXPECT_EQ(test_msg.size(), client_.Write(test_msg));

  source_->TransferData(ctx_.get(), SocketTraceConnector::kConnStatsTableNum, &data_table_);
  std::vector<TaggedRecordBatch> tablets = data_table_.ConsumeRecords();
  if (!tablets.empty()) {
    types::ColumnWrapperRecordBatch record_batch = tablets[0].records;

    auto indices = FindRecordIdxMatchesPID(record_batch, kPGSQLUPIDIdx, getpid());
    ASSERT_THAT(indices, IsEmpty());
  }
}

}  // namespace stirling
}  // namespace pl