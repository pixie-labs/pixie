/*
 * Copyright 2018- The Pixie Authors.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "src/common/exec/subprocess.h"
#include "src/common/testing/testing.h"
#include "src/stirling/core/output.h"
#include "src/stirling/source_connectors/socket_tracer/testing/client_server_system.h"
#include "src/stirling/source_connectors/socket_tracer/testing/socket_trace_bpf_test_fixture.h"
#include "src/stirling/testing/common.h"

namespace px {
namespace stirling {

using ::px::stirling::testing::AccessRecordBatch;
using ::px::stirling::testing::ClientServerSystem;
using ::px::stirling::testing::ColWrapperSizeIs;
using ::px::stirling::testing::FindRecordIdxMatchesPID;
using ::px::stirling::testing::FindRecordsMatchingPID;
using ::px::stirling::testing::SendRecvScript;
using ::px::stirling::testing::TCPSocket;
using ::testing::Each;
using ::testing::IsEmpty;
using ::testing::SizeIs;
using ::testing::UnorderedElementsAre;

constexpr int kUPIDIdx = conn_stats_idx::kUPID;
constexpr int kConnOpenIdx = conn_stats_idx::kConnOpen;
constexpr int kConnCloseIdx = conn_stats_idx::kConnClose;
constexpr int kBytesSentIdx = conn_stats_idx::kBytesSent;
constexpr int kBytesRecvIdx = conn_stats_idx::kBytesRecv;
constexpr int kAddrFamilyIdx = conn_stats_idx::kAddrFamily;
constexpr int kProtocolIdx = conn_stats_idx::kProtocol;
constexpr int kRoleIdx = conn_stats_idx::kRole;

// Turn off client-side tracing. Unlike data tracing, conn-stats should still trace client-side even
// with the client-side tracing turned off.
class ConnStatsBPFTest : public testing::SocketTraceBPFTest</* TClientSideTracing */ false> {
 public:
  ConnStatsBPFTest() { FLAGS_stirling_conn_stats_sampling_ratio = 1; }
};

TEST_F(ConnStatsBPFTest, UnclassifiedEvents) {
  StartTransferDataThread();

  SendRecvScript script = {{{"req1"}, {"resp1"}}, {{"req2"}, {"resp2"}}};

  // The server needs to be slow, so that the client is alive long enough for it to be discovered
  // by the TransferDataThread.
  std::chrono::milliseconds server_response_latency{200};

  ClientServerSystem cs(server_response_latency);
  cs.RunClientServer<&TCPSocket::Read, &TCPSocket::Write>(script);

  StopTransferDataThread();

  std::vector<TaggedRecordBatch> tablets = ConsumeRecords(SocketTraceConnector::kConnStatsTableNum);
  ASSERT_FALSE(tablets.empty());
  types::ColumnWrapperRecordBatch rb = tablets[0].records;
  PrintRecordBatch("test", kConnStatsTable.ToProto(), rb);

  // Check server-side stats.
  {
    auto indices = FindRecordIdxMatchesPID(rb, kUPIDIdx, cs.ServerPID());
    ASSERT_FALSE(indices.empty());

    // ConnStats may have produced various updates during the lifetime of the ClientServerSystem.
    // Grab the last record, which has the final information.
    int idx = indices.back();

    int conn_open = AccessRecordBatch<types::Int64Value>(rb, kConnOpenIdx, idx).val;
    int conn_close = AccessRecordBatch<types::Int64Value>(rb, kConnCloseIdx, idx).val;
    int bytes_sent = AccessRecordBatch<types::Int64Value>(rb, kBytesSentIdx, idx).val;
    int bytes_rcvd = AccessRecordBatch<types::Int64Value>(rb, kBytesRecvIdx, idx).val;
    int addr_family = AccessRecordBatch<types::Int64Value>(rb, kAddrFamilyIdx, idx).val;
    int protocol = AccessRecordBatch<types::Int64Value>(rb, kProtocolIdx, idx).val;
    int role = AccessRecordBatch<types::Int64Value>(rb, kRoleIdx, idx).val;

    EXPECT_THAT(conn_open, 1);
    EXPECT_THAT(conn_close, 1);
    EXPECT_THAT(bytes_sent, 10);
    EXPECT_THAT(bytes_rcvd, 8);
    EXPECT_THAT(addr_family, static_cast<int>(SockAddrFamily::kIPv4));
    EXPECT_THAT(protocol, kProtocolUnknown);
    EXPECT_THAT(role, kRoleServer);
  }

  // Check client-side stats.
  {
    auto indices = FindRecordIdxMatchesPID(rb, kUPIDIdx, cs.ClientPID());
    ASSERT_FALSE(indices.empty());

    // ConnStats may have produced various updates during the lifetime of the ClientServerSystem.
    // Grab the last record, which has the final information.
    int idx = indices.back();

    int conn_open = AccessRecordBatch<types::Int64Value>(rb, kConnOpenIdx, idx).val;
    int conn_close = AccessRecordBatch<types::Int64Value>(rb, kConnCloseIdx, idx).val;
    int bytes_sent = AccessRecordBatch<types::Int64Value>(rb, kBytesSentIdx, idx).val;
    int bytes_rcvd = AccessRecordBatch<types::Int64Value>(rb, kBytesRecvIdx, idx).val;
    int addr_family = AccessRecordBatch<types::Int64Value>(rb, kAddrFamilyIdx, idx).val;
    int protocol = AccessRecordBatch<types::Int64Value>(rb, kProtocolIdx, idx).val;
    int role = AccessRecordBatch<types::Int64Value>(rb, kRoleIdx, idx).val;

    EXPECT_THAT(conn_open, 1);
    EXPECT_THAT(conn_close, 1);
    EXPECT_THAT(bytes_sent, 8);
    EXPECT_THAT(bytes_rcvd, 10);
    EXPECT_THAT(addr_family, static_cast<int>(SockAddrFamily::kIPv4));
    EXPECT_THAT(protocol, kProtocolUnknown);
    EXPECT_THAT(role, kRoleClient);
  }
}

// This test checks that BPF is recording the role properly with only the accept/connect syscalls.
// Expectation is that summary stats are collected. The bytes transferred will be zero,
// but we should see the connect/accept for both client and server.
TEST_F(ConnStatsBPFTest, RoleFromConnectAccept) {
  StartTransferDataThread();

  // No data transfer, since we want to see if we can infer role from connect/accept syscalls.
  testing::SendRecvScript script({});

  // The server needs to be slow, so that the client is alive long enough for it to be discovered
  // by the TransferDataThread.
  std::chrono::milliseconds server_response_latency{200};

  ClientServerSystem system(server_response_latency);
  system.RunClientServer<&TCPSocket::Recv, &TCPSocket::Send>(script);

  StopTransferDataThread();

  std::vector<TaggedRecordBatch> tablets = ConsumeRecords(SocketTraceConnector::kConnStatsTableNum);
  ASSERT_FALSE(tablets.empty());

  PrintRecordBatch("test", kConnStatsTable.ToProto(), tablets[0].records);

  // Check client-side.
  {
    types::ColumnWrapperRecordBatch records =
        FindRecordsMatchingPID(tablets[0].records, kUPIDIdx, system.ClientPID());

    ASSERT_THAT(records, Each(ColWrapperSizeIs(1)));
    EXPECT_THAT(records[kProtocolIdx]->Get<types::Int64Value>(0), kProtocolUnknown);
    EXPECT_THAT(records[kRoleIdx]->Get<types::Int64Value>(0), kRoleClient);
    EXPECT_THAT(records[kConnOpenIdx]->Get<types::Int64Value>(0), 1);
    EXPECT_THAT(records[kConnCloseIdx]->Get<types::Int64Value>(0), 1);
    EXPECT_THAT(records[kBytesSentIdx]->Get<types::Int64Value>(0), 0);
    EXPECT_THAT(records[kBytesRecvIdx]->Get<types::Int64Value>(0), 0);
  }

  // Check server-side.
  {
    types::ColumnWrapperRecordBatch records =
        FindRecordsMatchingPID(tablets[0].records, kUPIDIdx, system.ServerPID());

    ASSERT_THAT(records, Each(ColWrapperSizeIs(1)));
    EXPECT_THAT(records[kProtocolIdx]->Get<types::Int64Value>(0), kProtocolUnknown);
    EXPECT_THAT(records[kRoleIdx]->Get<types::Int64Value>(0), kRoleServer);
    EXPECT_THAT(records[kConnOpenIdx]->Get<types::Int64Value>(0), 1);
    EXPECT_THAT(records[kConnCloseIdx]->Get<types::Int64Value>(0), 1);
    EXPECT_THAT(records[kBytesSentIdx]->Get<types::Int64Value>(0), 0);
    EXPECT_THAT(records[kBytesRecvIdx]->Get<types::Int64Value>(0), 0);
  }
}

// Test fixture that starts SocketTraceConnector after the connection was already established.
class ConnStatsMidConnBPFTest : public testing::SocketTraceBPFTest</* TClientSideTracing */ false> {
 protected:
  void SetUp() override {
    LOG(INFO) << absl::Substitute("Test PID = $0", getpid());
    // Uncomment to enable tracing:
    // FLAGS_stirling_conn_trace_pid = getpid();

    server_listener_.BindAndListen();
    client_.Connect(server_listener_);
    server_ = server_listener_.Accept();

    // Now SocketTraceConnector is created after the connection was already established.
    SocketTraceBPFTest::SetUp();
  }

  void TearDown() override {
    client_.Close();
    server_listener_.Close();
    if (server_ != nullptr) {
      server_->Close();
    }
    SocketTraceBPFTest::TearDown();
  }

  TCPSocket server_listener_;
  std::unique_ptr<TCPSocket> server_;

  TCPSocket client_;
};

TEST_F(ConnStatsMidConnBPFTest, DidNotSeeConnEstablishment) {
  StartTransferDataThread();

  std::string_view test_msg = "Hello World!";
  EXPECT_EQ(test_msg.size(), client_.Send(test_msg));
  std::string text;
  while (!server_->Recv(&text)) {
  }

  StopTransferDataThread();

  std::vector<TaggedRecordBatch> tablets = ConsumeRecords(SocketTraceConnector::kConnStatsTableNum);
  if (!tablets.empty()) {
    types::ColumnWrapperRecordBatch record_batch = tablets[0].records;

    auto indices = FindRecordIdxMatchesPID(record_batch, conn_stats_idx::kUPID, getpid());
    ASSERT_THAT(indices, IsEmpty());
  }
}

TEST_F(ConnStatsMidConnBPFTest, InferRole) {
  StartTransferDataThread();

  std::string_view test_msg = "Hello World!";
  EXPECT_EQ(test_msg.size(), client_.Send(test_msg));
  std::string text;
  while (!server_->Read(&text)) {
  }

  // Make sure traffic spans across multiple TransferData calls,
  // so that connection inference has a chance to kick in.
  // If this test becomes flaky, may want to try bumping this up to 4.
  for (int i = 0; i < 3; ++i) {
    EXPECT_EQ(test_msg.size(), client_.Send(test_msg));
    while (!server_->Recv(&text)) {
    }
    std::this_thread::sleep_for(kTransferDataPeriod);
  }

  StopTransferDataThread();

  std::vector<TaggedRecordBatch> tablets = ConsumeRecords(SocketTraceConnector::kConnStatsTableNum);

  ASSERT_FALSE(tablets.empty());

  PrintRecordBatch("test", kConnStatsTable.ToProto(), tablets[0].records);

  // Check client-side.
  {
    types::ColumnWrapperRecordBatch records =
        FindRecordsMatchingPID(tablets[0].records, kUPIDIdx, getpid());

    ASSERT_THAT(records, Each(ColWrapperSizeIs(2)));

    // One of the records is the server and the other is the client.
    int s_idx;
    int c_idx;

    if (records[kRoleIdx]->Get<types::Int64Value>(0).val == kRoleClient) {
      c_idx = 0;
      s_idx = 1;
    } else {
      c_idx = 1;
      s_idx = 0;
    }

    // Check that we properly found a client and server record.
    EXPECT_THAT(records[kRoleIdx]->Get<types::Int64Value>(c_idx), kRoleClient);
    EXPECT_THAT(records[kRoleIdx]->Get<types::Int64Value>(s_idx), kRoleServer);

    // Check client record.
    EXPECT_THAT(records[kProtocolIdx]->Get<types::Int64Value>(c_idx), kProtocolUnknown);
    EXPECT_THAT(records[kConnOpenIdx]->Get<types::Int64Value>(c_idx), 1);
    EXPECT_THAT(records[kConnCloseIdx]->Get<types::Int64Value>(c_idx), 0);
    // TODO(oazizi): Investigate why this is not reliable.
    // EXPECT_THAT(records[kBytesSentIdx]->Get<types::Int64Value>(c_idx), 60);
    EXPECT_THAT(records[kBytesRecvIdx]->Get<types::Int64Value>(c_idx), 0);

    // Check server record.
    EXPECT_THAT(records[kProtocolIdx]->Get<types::Int64Value>(s_idx), kProtocolUnknown);
    EXPECT_THAT(records[kConnOpenIdx]->Get<types::Int64Value>(s_idx), 1);
    EXPECT_THAT(records[kConnCloseIdx]->Get<types::Int64Value>(s_idx), 0);
    EXPECT_THAT(records[kBytesSentIdx]->Get<types::Int64Value>(s_idx), 0);
    // TODO(oazizi): Investigate why this is not reliable.
    // EXPECT_THAT(records[kBytesRecvIdx]->Get<types::Int64Value>(s_idx), 60);
  }
}

}  // namespace stirling
}  // namespace px
