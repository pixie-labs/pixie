#pragma once

#ifndef __linux__

#include "src/stirling/core/source_connector.h"

namespace pl {
namespace stirling {

DUMMY_SOURCE_CONNECTOR(SocketTraceConnector);

}  // namespace stirling
}  // namespace pl

#else

#include <fstream>
#include <list>
#include <map>
#include <memory>
#include <set>
#include <string>
#include <utility>
#include <vector>

#include <absl/synchronization/mutex.h>

#include "demos/applications/hipster_shop/reflection.h"
#include "src/common/grpcutils/service_descriptor_database.h"
#include "src/common/system/socket_info.h"
#include "src/stirling/bpf_tools/bcc_wrapper.h"
#include "src/stirling/obj_tools/dwarf_tools.h"
#include "src/stirling/obj_tools/elf_tools.h"

#include "src/stirling/core/source_connector.h"
#include "src/stirling/source_connectors/socket_tracer/bcc_bpf_intf/socket_trace.hpp"
#include "src/stirling/source_connectors/socket_tracer/conn_trackers_manager.h"
#include "src/stirling/source_connectors/socket_tracer/connection_stats.h"
#include "src/stirling/source_connectors/socket_tracer/connection_tracker.h"
#include "src/stirling/source_connectors/socket_tracer/socket_trace_bpf_tables.h"
#include "src/stirling/source_connectors/socket_tracer/socket_trace_tables.h"
#include "src/stirling/source_connectors/socket_tracer/uprobe_manager.h"
#include "src/stirling/utils/proc_path_tools.h"
#include "src/stirling/utils/proc_tracker.h"

DECLARE_bool(stirling_enable_parsing_protobufs);
DECLARE_uint32(stirling_socket_trace_sampling_period_millis);
DECLARE_string(perf_buffer_events_output_path);
DECLARE_bool(stirling_enable_http_tracing);
DECLARE_bool(stirling_enable_http2_tracing);
DECLARE_bool(stirling_enable_mysql_tracing);
DECLARE_bool(stirling_enable_cass_tracing);
DECLARE_bool(stirling_enable_dns_tracing);
DECLARE_bool(stirling_enable_redis_tracing);
DECLARE_bool(stirling_disable_self_tracing);
DECLARE_string(stirling_role_to_trace);

namespace pl {
namespace stirling {

class SocketTraceConnector : public SourceConnector, public bpf_tools::BCCWrapper {
 public:
  static constexpr auto kTables = MakeArray(kConnStatsTable, kHTTPTable, kMySQLTable, kCQLTable,
                                            kPGSQLTable, kDNSTable, kRedisTable);

  static constexpr uint32_t kConnStatsTableNum = TableNum(kTables, kConnStatsTable);
  static constexpr uint32_t kHTTPTableNum = TableNum(kTables, kHTTPTable);
  static constexpr uint32_t kMySQLTableNum = TableNum(kTables, kMySQLTable);
  static constexpr uint32_t kCQLTableNum = TableNum(kTables, kCQLTable);
  static constexpr uint32_t kPGSQLTableNum = TableNum(kTables, kPGSQLTable);
  static constexpr uint32_t kDNSTableNum = TableNum(kTables, kDNSTable);
  static constexpr uint32_t kRedisTableNum = TableNum(kTables, kRedisTable);

  static std::unique_ptr<SourceConnector> Create(std::string_view name) {
    return std::unique_ptr<SourceConnector>(new SocketTraceConnector(name));
  }

  Status InitImpl() override;
  Status StopImpl() override;
  void InitContextImpl(ConnectorContext* ctx) override;
  void TransferDataImpl(ConnectorContext* ctx, uint32_t table_num, DataTable* data_table) override;

  // Perform actions that are not specifically targeting a table.
  // For example, drain perf buffers, deploy new uprobes, and update socket info manager.
  // If these were performed on every TransferData(), they would occur too frequently,
  // because TransferData() gets called for every table in the connector.
  // That would then cause performance overheads.
  void UpdateCommonState(ConnectorContext* ctx);

  // A wrapper around UpdateCommonState that filters out calls to UpdateCommonState()
  // if the state had already been updated for a different table.
  // A second call to this function for any table will trigger a call to UpdateCommonState(),
  // so this effectively means that UpdateCommonState() runs as frequently as the most frequently
  // transferred table.
  void CachedUpdateCommonState(ConnectorContext* ctx, uint32_t table_num);

  // Updates control map value for protocol, which specifies which role(s) to trace for the given
  // protocol's traffic.
  //
  // Role_mask a bit mask, and represents the EndpointRole roles that are allowed to transfer
  // data from inside BPF to user-space.
  Status UpdateBPFProtocolTraceRole(TrafficProtocol protocol, uint64_t role_mask);
  Status TestOnlySetTargetPID(int64_t pid);
  Status DisableSelfTracing();

  /**
   * Gets a pointer to the most recent ConnectionTracker for the given pid and fd.
   *
   * @return Pointer to the ConnectionTracker, or error::NotFound if it does not exist.
   */
  StatusOr<const ConnectionTracker*> GetConnectionTracker(uint32_t pid, uint32_t fd) const {
    return conn_trackers_.GetConnectionTracker(pid, fd);
  }

 private:
  // ReadPerfBuffers poll callback functions (must be static).
  // These are used by the static variables below, and have to be placed here.
  static void HandleDataEvent(void* cb_cookie, void* data, int data_size);
  static void HandleDataEventLoss(void* cb_cookie, uint64_t lost);
  static void HandleControlEvent(void* cb_cookie, void* data, int data_size);
  static void HandleControlEventLoss(void* cb_cookie, uint64_t lost);
  static void HandleMMapEvent(void* cb_cookie, void* data, int data_size);
  static void HandleMMapEventLoss(void* cb_cookie, uint64_t lost);
  static void HandleHTTP2HeaderEvent(void* cb_cookie, void* data, int data_size);
  static void HandleHTTP2HeaderEventLoss(void* cb_cookie, uint64_t lost);
  static void HandleHTTP2Data(void* cb_cookie, void* data, int data_size);
  static void HandleHTTP2DataLoss(void* cb_cookie, uint64_t lost);

  static constexpr auto kProbeSpecs = MakeArray<bpf_tools::KProbeSpec>({
      {"connect", bpf_tools::BPFProbeAttachType::kEntry, "syscall__probe_entry_connect"},
      {"connect", bpf_tools::BPFProbeAttachType::kReturn, "syscall__probe_ret_connect"},
      {"accept", bpf_tools::BPFProbeAttachType::kEntry, "syscall__probe_entry_accept"},
      {"accept", bpf_tools::BPFProbeAttachType::kReturn, "syscall__probe_ret_accept"},
      {"accept4", bpf_tools::BPFProbeAttachType::kEntry, "syscall__probe_entry_accept4"},
      {"accept4", bpf_tools::BPFProbeAttachType::kReturn, "syscall__probe_ret_accept4"},
      {"open", bpf_tools::BPFProbeAttachType::kReturn, "syscall__probe_ret_open"},
      {"creat", bpf_tools::BPFProbeAttachType::kReturn, "syscall__probe_ret_open"},
      {"openat", bpf_tools::BPFProbeAttachType::kReturn, "syscall__probe_ret_open"},
      {"write", bpf_tools::BPFProbeAttachType::kEntry, "syscall__probe_entry_write"},
      {"write", bpf_tools::BPFProbeAttachType::kReturn, "syscall__probe_ret_write"},
      {"writev", bpf_tools::BPFProbeAttachType::kEntry, "syscall__probe_entry_writev"},
      {"writev", bpf_tools::BPFProbeAttachType::kReturn, "syscall__probe_ret_writev"},
      {"send", bpf_tools::BPFProbeAttachType::kEntry, "syscall__probe_entry_send"},
      {"send", bpf_tools::BPFProbeAttachType::kReturn, "syscall__probe_ret_send"},
      {"sendto", bpf_tools::BPFProbeAttachType::kEntry, "syscall__probe_entry_sendto"},
      {"sendto", bpf_tools::BPFProbeAttachType::kReturn, "syscall__probe_ret_sendto"},
      {"sendmsg", bpf_tools::BPFProbeAttachType::kEntry, "syscall__probe_entry_sendmsg"},
      {"sendmsg", bpf_tools::BPFProbeAttachType::kReturn, "syscall__probe_ret_sendmsg"},
      {"sendmmsg", bpf_tools::BPFProbeAttachType::kEntry, "syscall__probe_entry_sendmmsg"},
      {"sendmmsg", bpf_tools::BPFProbeAttachType::kReturn, "syscall__probe_ret_sendmmsg"},
      {"read", bpf_tools::BPFProbeAttachType::kEntry, "syscall__probe_entry_read"},
      {"read", bpf_tools::BPFProbeAttachType::kReturn, "syscall__probe_ret_read"},
      {"readv", bpf_tools::BPFProbeAttachType::kEntry, "syscall__probe_entry_readv"},
      {"readv", bpf_tools::BPFProbeAttachType::kReturn, "syscall__probe_ret_readv"},
      {"recv", bpf_tools::BPFProbeAttachType::kEntry, "syscall__probe_entry_recv"},
      {"recv", bpf_tools::BPFProbeAttachType::kReturn, "syscall__probe_ret_recv"},
      {"recvfrom", bpf_tools::BPFProbeAttachType::kEntry, "syscall__probe_entry_recvfrom"},
      {"recvfrom", bpf_tools::BPFProbeAttachType::kReturn, "syscall__probe_ret_recvfrom"},
      {"recvmsg", bpf_tools::BPFProbeAttachType::kEntry, "syscall__probe_entry_recvmsg"},
      {"recvmsg", bpf_tools::BPFProbeAttachType::kReturn, "syscall__probe_ret_recvmsg"},
      {"recvmmsg", bpf_tools::BPFProbeAttachType::kEntry, "syscall__probe_entry_recvmmsg"},
      {"recvmmsg", bpf_tools::BPFProbeAttachType::kReturn, "syscall__probe_ret_recvmmsg"},
      {"close", bpf_tools::BPFProbeAttachType::kEntry, "syscall__probe_entry_close"},
      {"close", bpf_tools::BPFProbeAttachType::kReturn, "syscall__probe_ret_close"},
      {"mmap", bpf_tools::BPFProbeAttachType::kEntry, "syscall__probe_entry_mmap"},
  });

  // TODO(oazizi): Remove send and recv probes once we are confident that they don't trace anything.
  //               Note that send/recv are not in the syscall table
  //               (https://filippo.io/linux-syscall-table/), but are defined as SYSCALL_DEFINE4 in
  //               https://elixir.bootlin.com/linux/latest/source/net/socket.c.

  inline static const auto kPerfBufferSpecs = MakeArray<bpf_tools::PerfBufferSpec>({
      // For data events. The order must be consistent with output tables.
      {"socket_data_events", HandleDataEvent, HandleDataEventLoss},
      // For non-data events. Must not mix with the above perf buffers for data events.
      {"socket_control_events", HandleControlEvent, HandleControlEventLoss},
      {"mmap_events", HandleMMapEvent, HandleMMapEventLoss},
      {"go_grpc_header_events", HandleHTTP2HeaderEvent, HandleHTTP2HeaderEventLoss},
      {"go_grpc_data_events", HandleHTTP2Data, HandleHTTP2DataLoss},
  });

  // Most HTTP servers support 8K headers, so we truncate after that.
  // https://stackoverflow.com/questions/686217/maximum-on-http-header-values
  inline static constexpr size_t kMaxHTTPHeadersBytes = 8192;

  // Only sample the head of the body, to save space.
  inline static constexpr size_t kMaxBodyBytes = 512;

  // TODO(yzhao): We will remove this once finalized the mechanism of lazy protobuf parse.
  inline static ::pl::grpc::ServiceDescriptorDatabase grpc_desc_db_{
      demos::hipster_shop::GetFileDescriptorSet()};

  explicit SocketTraceConnector(std::string_view source_name);

  // Initialize protocol_transfer_specs_.
  void InitProtocolTransferSpecs();

  // Events from BPF.
  // TODO(oazizi/yzhao): These all operate based on pass-by-value, which copies.
  //                     The Handle* functions should call make_unique() of new corresponding
  //                     objects, and these functions should take unique_ptrs.
  void AcceptDataEvent(std::unique_ptr<SocketDataEvent> event);
  void AcceptControlEvent(socket_control_event_t event);
  void AcceptHTTP2Header(std::unique_ptr<HTTP2HeaderEvent> event);
  void AcceptHTTP2Data(std::unique_ptr<HTTP2DataEvent> event);

  // Transfer of messages to the data table.
  void TransferStreams(ConnectorContext* ctx, uint32_t table_num, DataTable* data_table);
  void TransferConnectionStats(ConnectorContext* ctx, DataTable* data_table);

  template <typename TProtocolTraits>
  void TransferStream(ConnectorContext* ctx, ConnectionTracker* tracker, DataTable* data_table);

  template <typename TRecordType>
  static void AppendMessage(ConnectorContext* ctx, const ConnectionTracker& conn_tracker,
                            TRecordType record, DataTable* data_table);

  std::thread RunDeployUProbesThread(const absl::flat_hash_set<md::UPID>& pids);

  // Returns vector representing currently known cluster (pod and service) CIDRs.
  std::vector<CIDRBlock> ClusterCIDRs(ConnectorContext* ctx);

  // Setups output file stream object writing to the input file path.
  void SetupOutput(const std::filesystem::path& file);
  // Writes data event to the specified output file.
  void WriteDataEvent(const SocketDataEvent& event);

  ConnTrackersManager conn_trackers_;

  ConnectionStats connection_stats_;

  struct TransferSpec {
    bool enabled = false;
    uint32_t table_num = 0;
    std::vector<EndpointRole> trace_roles;
    std::function<void(SocketTraceConnector&, ConnectorContext*, ConnectionTracker*, DataTable*)>
        transfer_fn = nullptr;
  };

  // This map controls how each protocol is processed and transferred.
  // The table num identifies which data the collected data is transferred.
  // The transfer_fn defines which function is called to process the data for transfer.
  std::vector<TransferSpec> protocol_transfer_specs_;

  // Keep track of which tables have had data transferred via TransferData()
  // since the last UpdateCommonState().
  // Used as a performance optimization to avoid too many calls to UpdateCommonState().
  // Essentially, we allow a TransferData() for one table to piggy-back on the UpdateCommonState()
  // of another table.
  // A bit being set means that the table in question should skip a call to UpdateCommonState().
  std::bitset<kTables.size()> table_access_history_;

  // Keep track of when the last perf buffer drain event was triggered.
  // Perf buffer draining is not atomic nor synchronous, so we want the time before draining.
  // The time is used by DataTable to produce records in sorted order across iterations.
  //   Example: data_table->SetConsumeRecordsCutoffTime(perf_buffer_drain_time_);
  uint64_t perf_buffer_drain_time_ = 0;

  // If not a nullptr, writes the events received from perf buffers to this stream.
  std::unique_ptr<std::ofstream> perf_buffer_events_output_stream_;
  enum class OutputFormat {
    kTxt,
    kBin,
  };
  OutputFormat perf_buffer_events_output_format_ = OutputFormat::kTxt;

  // Portal to query for connections, by pid and inode.
  std::unique_ptr<system::SocketInfoManager> socket_info_mgr_;

  std::unique_ptr<system::ProcParser> proc_parser_;

  std::shared_ptr<ConnInfoMapManager> conn_info_map_mgr_;

  UProbeManager uprobe_mgr_;
  std::atomic<int> num_deploy_uprobes_threads_ = 0;

  FRIEND_TEST(SocketTraceConnectorTest, AppendNonContiguousEvents);
  FRIEND_TEST(SocketTraceConnectorTest, NoEvents);
  FRIEND_TEST(SocketTraceConnectorTest, SortedByResponseTime);
  FRIEND_TEST(SocketTraceConnectorTest, HTTPBasic);
  FRIEND_TEST(SocketTraceConnectorTest, HTTPContentType);
  FRIEND_TEST(SocketTraceConnectorTest, UPIDCheck);
  FRIEND_TEST(SocketTraceConnectorTest, RequestResponseMatching);
  FRIEND_TEST(SocketTraceConnectorTest, MissingEventInStream);
  FRIEND_TEST(SocketTraceConnectorTest, ConnectionCleanupInOrder);
  FRIEND_TEST(SocketTraceConnectorTest, ConnectionCleanupOutOfOrder);
  FRIEND_TEST(SocketTraceConnectorTest, ConnectionCleanupMissingDataEvent);
  FRIEND_TEST(SocketTraceConnectorTest, ConnectionCleanupOldGenerations);
  FRIEND_TEST(SocketTraceConnectorTest, ConnectionCleanupInactiveDead);
  FRIEND_TEST(SocketTraceConnectorTest, ConnectionCleanupInactiveAlive);
  FRIEND_TEST(SocketTraceConnectorTest, ConnectionCleanupNoProtocol);
  FRIEND_TEST(SocketTraceConnectorTest, MySQLPrepareExecuteClose);
  FRIEND_TEST(SocketTraceConnectorTest, MySQLQuery);
  FRIEND_TEST(SocketTraceConnectorTest, MySQLMultipleCommands);
  FRIEND_TEST(SocketTraceConnectorTest, MySQLQueryWithLargeResultset);
  FRIEND_TEST(SocketTraceConnectorTest, MySQLMultiResultset);
  FRIEND_TEST(SocketTraceConnectorTest, CQLQuery);
  FRIEND_TEST(SocketTraceConnectorTest, HTTP2ClientTest);
  FRIEND_TEST(SocketTraceConnectorTest, HTTP2ServerTest);
  FRIEND_TEST(SocketTraceConnectorTest, HTTP2PartialStream);
  FRIEND_TEST(SocketTraceConnectorTest, HTTP2ResponseOnly);
  FRIEND_TEST(SocketTraceConnectorTest, HTTP2SpanAcrossTransferData);
  FRIEND_TEST(SocketTraceConnectorTest, HTTP2SequentialStreams);
  FRIEND_TEST(SocketTraceConnectorTest, HTTP2ParallelStreams);
  FRIEND_TEST(SocketTraceConnectorTest, HTTP2StreamSandwich);
  FRIEND_TEST(SocketTraceConnectorTest, HTTP2StreamIDRace);
  FRIEND_TEST(SocketTraceConnectorTest, HTTP2OldStream);
};

}  // namespace stirling
}  // namespace pl

#endif