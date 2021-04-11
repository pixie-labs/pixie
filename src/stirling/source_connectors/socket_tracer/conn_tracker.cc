#include "src/stirling/source_connectors/socket_tracer/conn_tracker.h"

#include <arpa/inet.h>
#include <netinet/in.h>
#include <unistd.h>

#include <algorithm>
#include <chrono>
#include <deque>
#include <filesystem>
#include <numeric>
#include <vector>

#include <absl/strings/numbers.h>
#include <magic_enum.hpp>

#include "src/common/base/inet_utils.h"
#include "src/common/system/socket_info.h"
#include "src/common/system/system.h"
#include "src/stirling/source_connectors/socket_tracer/bcc_bpf_intf/go_grpc_types.hpp"
#include "src/stirling/source_connectors/socket_tracer/conn_stats.h"
#include "src/stirling/source_connectors/socket_tracer/conn_trackers_manager.h"

constexpr int64_t kUnsetPIDFD = -1;
DEFINE_int64(stirling_conn_trace_pid, kUnsetPIDFD, "Trace activity on this pid.");
DEFINE_int64(stirling_conn_trace_fd, kUnsetPIDFD, "Trace activity on this fd.");
DEFINE_bool(
    stirling_conn_disable_to_bpf, true,
    "Send information about connection tracking disablement to BPF, so it can stop sending data.");
DEFINE_int64(
    stirling_check_proc_for_conn_close, true,
    "If enabled, Stirling will check Linux /proc on idle connections to see if they are closed.");

DECLARE_int32(test_only_socket_trace_target_pid);

namespace pl {
namespace stirling {

// Parse failure rate threshold, after which a connection tracker will be disabled.
constexpr double kParseFailureRateThreshold = 0.4;
constexpr double kStitchFailureRateThreshold = 0.5;

//--------------------------------------------------------------
// ConnTracker
//--------------------------------------------------------------

ConnTracker::~ConnTracker() {
  CONN_TRACE(2) << "Being destroyed";
  if (conn_info_map_mgr_ != nullptr) {
    conn_info_map_mgr_->ReleaseResources(conn_id_);
  }
}

void ConnTracker::AddControlEvent(const socket_control_event_t& event) {
  switch (event.type) {
    case kConnOpen:
      AddConnOpenEvent(event.open);
      break;
    case kConnClose:
      AddConnCloseEvent(event.close);
      break;
    default:
      LOG(DFATAL) << "Unknown control event type: " << event.type;
  }
}

void ConnTracker::AddConnOpenEvent(const conn_event_t& conn_event) {
  if (open_info_.timestamp_ns != 0) {
    LOG_FIRST_N(WARNING, 20) << absl::Substitute("[PL-985] Clobbering existing ConnOpenEvent $0.",
                                                 ToString(conn_event.conn_id));
  }

  SetConnID(conn_event.conn_id);

  CheckTracker();

  UpdateTimestamps(conn_event.timestamp_ns);

  open_info_.timestamp_ns = conn_event.timestamp_ns;

  PopulateSockAddr(reinterpret_cast<const struct sockaddr*>(&conn_event.addr),
                   &open_info_.remote_addr);

  SetRole(conn_event.role, "inferred from conn_open");

  CONN_TRACE(1) << absl::Substitute("conn_open af=$0 addr=$1",
                                    magic_enum::enum_name(open_info_.remote_addr.family),
                                    open_info_.remote_addr.AddrStr());

  if (ShouldExportToConnStats()) {
    ExportInitialConnStats();
  }
}

void ConnTracker::AddConnCloseEvent(const close_event_t& close_event) {
  if (close_info_.timestamp_ns != 0) {
    LOG_FIRST_N(ERROR, 20) << absl::Substitute("Clobbering existing ConnCloseEvent $0.",
                                               ToString(close_event.conn_id));
  }

  SetConnID(close_event.conn_id);

  CheckTracker();

  UpdateTimestamps(close_event.timestamp_ns);

  close_info_.timestamp_ns = close_event.timestamp_ns;
  close_info_.send_bytes = close_event.wr_bytes;
  close_info_.recv_bytes = close_event.rd_bytes;

  CONN_TRACE(1) << absl::Substitute("conn_close");

  MarkForDeath();
}

void ConnTracker::AddDataEvent(std::unique_ptr<SocketDataEvent> event) {
  SetConnID(event->attr.conn_id);
  bool role_changed = SetRole(event->attr.traffic_class.role, "inferred from data_event");
  SetProtocol(event->attr.traffic_class.protocol, "inferred from data_event");

  // If role has just resolved, it may be time to inform conn stats.
  if (role_changed && ShouldExportToConnStats()) {
    ExportInitialConnStats();
  }

  CheckTracker();
  UpdateTimestamps(event->attr.timestamp_ns);

  // Only export metric to conn_stats_ after remote_endpoint has been resolved.
  if (ShouldExportToConnStats()) {
    // Export stats to ConnStats object.
    conn_stats_->AddDataEvent(*this, *event);
  }

  UpdateDataStats(*event);

  CONN_TRACE(1) << absl::Substitute("Data event received: $0", event->ToString());

  // TODO(yzhao): Change to let userspace resolve the connection type and signal back to BPF.
  // Then we need at least one data event to let ConnTracker know the field descriptor.
  if (event->attr.traffic_class.protocol == kProtocolUnknown) {
    return;
  }

  if (event->attr.traffic_class.protocol != traffic_class_.protocol) {
    return;
  }

  // A disabled tracker doesn't collect data events.
  if (state() == State::kDisabled) {
    return;
  }

  switch (event->attr.direction) {
    case TrafficDirection::kEgress: {
      send_data_.AddData(std::move(event));
    } break;
    case TrafficDirection::kIngress: {
      recv_data_.AddData(std::move(event));
    } break;
  }
}

protocols::http2::HalfStream* ConnTracker::HalfStreamPtr(uint32_t stream_id, bool write_event) {
  // Client-initiated streams have odd stream IDs.
  // Server-initiated streams have even stream IDs.
  // https://tools.ietf.org/html/rfc7540#section-5.1.1
  const bool client_stream = (stream_id % 2 == 1);
  HTTP2StreamsContainer& streams = client_stream ? http2_client_streams_ : http2_server_streams_;
  return streams.HalfStreamPtr(stream_id, write_event);
}

EndpointRole InferHTTP2Role(bool write_event, const std::unique_ptr<HTTP2HeaderEvent>& hdr) {
  // Look for standard headers to infer role.
  // Could look at others (:scheme, :path, :authority), but this seems sufficient.

  if (hdr->name == ":method") {
    return (write_event) ? kRoleClient : kRoleServer;
  }
  if (hdr->name == ":status") {
    return (write_event) ? kRoleServer : kRoleClient;
  }

  return kRoleUnknown;
}

void ConnTracker::AddHTTP2Header(std::unique_ptr<HTTP2HeaderEvent> hdr) {
  SetConnID(hdr->attr.conn_id);
  SetProtocol(kProtocolHTTP2, "inferred from http2 headers");

  if (traffic_class_.protocol != kProtocolHTTP2) {
    return;
  }

  CONN_TRACE(2) << absl::Substitute("HTTP2 header event received: $0", hdr->ToString());

  if (conn_id_.fd == 0) {
    Disable(
        "FD of zero is usually not valid. One reason for could be that net.Conn could not be "
        "accessed because of incorrect type information. If this is the case, the dwarf feature "
        "should fix this.");
  }

  // A disabled tracker doesn't collect data events.
  if (state() == State::kDisabled) {
    return;
  }

  CheckTracker();

  // Don't trace any control messages.
  if (hdr->attr.stream_id == 0) {
    return;
  }

  UpdateTimestamps(hdr->attr.timestamp_ns);

  bool write_event = false;
  switch (hdr->attr.type) {
    case HeaderEventType::kHeaderEventWrite:
      write_event = true;
      break;
    case HeaderEventType::kHeaderEventRead:
      write_event = false;
      break;
    default:
      LOG(WARNING) << "Unexpected event type";
      return;
  }

  if (traffic_class_.role == kRoleUnknown) {
    EndpointRole role = InferHTTP2Role(write_event, hdr);
    bool role_changed = SetRole(role, "Inferred from http2 header");
    if (role_changed && ShouldExportToConnStats()) {
      ExportInitialConnStats();
    }
  }

  protocols::http2::HalfStream* half_stream_ptr = HalfStreamPtr(hdr->attr.stream_id, write_event);

  // End stream flag is on a dummy header, so just record the end_stream, but don't add the headers.
  if (hdr->attr.end_stream) {
    // Expecting a dummy (empty) header since this is how it is done in BPF.
    ECHECK(hdr->name.empty());
    ECHECK(hdr->value.empty());

    // Only expect one end_stream signal per stream direction.
    // Note: Duplicate calls to the writeHeaders (calls with same arguments) have been observed.
    // It happens rarely, and root cause is unknown. It does not appear to be a retransmission due
    // to dropped packets (as that is handled by the TCP layer).
    // For now, just print a warning. The only harm is duplicated headers in the tables.
    // Note that the duplicates are not necessarily restricted to headers which have the end_stream
    // flag set; the end_stream cases are just the easiest to detect.
    if (half_stream_ptr->end_stream) {
      LOG_FIRST_N(WARNING, 10) << absl::Substitute(
          "Duplicate end_stream flag in header. stream_id: $0, conn_id: $1", hdr->attr.stream_id,
          ToString(hdr->attr.conn_id));
    }

    half_stream_ptr->end_stream = true;
    return;
  }

  half_stream_ptr->headers.emplace(std::move(hdr->name), std::move(hdr->value));
  half_stream_ptr->UpdateTimestamp(hdr->attr.timestamp_ns);
}

void ConnTracker::AddHTTP2Data(std::unique_ptr<HTTP2DataEvent> data) {
  SetConnID(data->attr.conn_id);
  SetProtocol(kProtocolHTTP2, "inferred from http2 data");

  if (traffic_class_.protocol != kProtocolHTTP2) {
    return;
  }

  CONN_TRACE(1) << absl::Substitute("HTTP2 data event received: $0", data->ToString());

  if (conn_id_.fd == 0) {
    Disable(
        "FD of zero is usually not valid. One reason for could be that net.Conn could not be "
        "accessed because of incorrect type information. If this is the case, the dwarf feature "
        "should fix this.");
  }

  // A disabled tracker doesn't collect data events.
  if (state() == State::kDisabled) {
    return;
  }

  CheckTracker();

  // Don't trace any control messages.
  if (data->attr.stream_id == 0) {
    return;
  }

  UpdateTimestamps(data->attr.timestamp_ns);

  bool write_event = false;
  switch (data->attr.type) {
    case DataFrameEventType::kDataFrameEventWrite:
      write_event = true;
      break;
    case DataFrameEventType::kDataFrameEventRead:
      write_event = false;
      break;
    default:
      LOG(WARNING) << "Unexpected event type";
      return;
  }

  protocols::http2::HalfStream* half_stream_ptr = HalfStreamPtr(data->attr.stream_id, write_event);

  // Note: Duplicate calls to the writeHeaders have been observed (though they are rare).
  // It is not yet known if duplicate data also occurs. This log will help us figure out if such
  // cases exist. Note that the duplicates are not related to the end_stream flag being set;
  // the end_stream cases are just the easiest to detect.
  if (half_stream_ptr->end_stream && data->attr.end_stream) {
    LOG_FIRST_N(WARNING, 10) << absl::Substitute(
        "Duplicate end_stream flag in data. stream_id: $0, conn_id: $1", data->attr.stream_id,
        ToString(data->attr.conn_id));
  }

  half_stream_ptr->data += data->payload;
  half_stream_ptr->end_stream |= data->attr.end_stream;
  half_stream_ptr->UpdateTimestamp(data->attr.timestamp_ns);
}

template <>
std::vector<protocols::http2::Record>
ConnTracker::ProcessToRecords<protocols::http2::ProtocolTraits>() {
  protocols::RecordsWithErrorCount<protocols::http2::Record> result;

  protocols::http2::ProcessHTTP2Streams(&http2_client_streams_, IsZombie(), &result);
  protocols::http2::ProcessHTTP2Streams(&http2_server_streams_, IsZombie(), &result);

  UpdateResultStats(result);
  Cleanup<protocols::http2::ProtocolTraits>();

  return std::move(result.records);
}

void ConnTracker::Reset() {
  send_data_.Reset();
  recv_data_.Reset();

  protocol_state_.reset();
}

void ConnTracker::Disable(std::string_view reason) {
  if (state_ != State::kDisabled) {
    if (conn_info_map_mgr_ != nullptr && FLAGS_stirling_conn_disable_to_bpf) {
      conn_info_map_mgr_->Disable(conn_id_);
    }
    CONN_TRACE(1) << absl::Substitute("Disabling connection dest=$0:$1 reason=$2",
                                      open_info_.remote_addr.AddrStr(),
                                      open_info_.remote_addr.port(), reason);
  }

  state_ = State::kDisabled;
  disable_reason_ = reason;

  Reset();
}

bool ConnTracker::AllEventsReceived() const {
  return close_info_.timestamp_ns != 0 &&
         stats_.Get(Stats::Key::kBytesSent) == close_info_.send_bytes &&
         stats_.Get(Stats::Key::kBytesRecv) == close_info_.recv_bytes;
}

void ConnTracker::SetConnID(struct conn_id_t conn_id) {
  DCHECK(conn_id_.upid.pid == 0 || conn_id_.upid.pid == conn_id.upid.pid) << absl::Substitute(
      "Mismatched conn info: tracker=$0 event=$1", ToString(conn_id_), ToString(conn_id));
  DCHECK(conn_id_.fd == 0 || conn_id_.fd == conn_id.fd) << absl::Substitute(
      "Mismatched conn info: tracker=$0 event=$1", ToString(conn_id_), ToString(conn_id));
  DCHECK(conn_id_.tsid == 0 || conn_id_.tsid == conn_id.tsid) << absl::Substitute(
      "Mismatched conn info: tracker=$0 event=$1", ToString(conn_id_), ToString(conn_id));
  DCHECK(conn_id_.upid.start_time_ticks == 0 ||
         conn_id_.upid.start_time_ticks == conn_id.upid.start_time_ticks)
      << absl::Substitute("Mismatched conn info: tracker=$0 event=$1", ToString(conn_id_),
                          ToString(conn_id));

  conn_id_ = conn_id;

  if (conn_id_.upid.pid == FLAGS_stirling_conn_trace_pid ||
      conn_id_.upid.pid == static_cast<uint32_t>(FLAGS_test_only_socket_trace_target_pid)) {
    if (FLAGS_stirling_conn_trace_fd == kUnsetPIDFD ||
        conn_id_.fd == FLAGS_stirling_conn_trace_fd) {
      SetDebugTrace(2);
    }
  }
}

bool ConnTracker::SetRole(EndpointRole role, std::string_view reason) {
  // Don't allow changing active role, unless it is from unknown to something else.
  if (traffic_class_.role != kRoleUnknown) {
    if (role != kRoleUnknown && traffic_class_.role != role) {
      CONN_TRACE(2) << absl::Substitute(
          "Not allowed to change the role of an active ConnTracker: $0, old role: $1, new "
          "role: $2",
          ToString(conn_id_), magic_enum::enum_name(traffic_class_.role),
          magic_enum::enum_name(role));
    }

    // Role did not change.
    return false;
  }

  if (role != kRoleUnknown) {
    CONN_TRACE(1) << absl::Substitute("Role updated $0 -> $1, reason=[$2]]",
                                      magic_enum::enum_name(traffic_class_.role),
                                      magic_enum::enum_name(role), reason);
    traffic_class_.role = role;
    return true;
  }

  // Role did not change.
  return false;
}

// Returns false if protocol change was not allowed.
bool ConnTracker::SetProtocol(TrafficProtocol protocol, std::string_view reason) {
  // No change, so we're all good.
  if (traffic_class_.protocol == protocol) {
    return true;
  }

  // Changing the active protocol of a connection tracker is not allowed.
  if (traffic_class_.protocol != kProtocolUnknown) {
    CONN_TRACE(2) << absl::Substitute(
        "Not allowed to change the protocol of an active ConnTracker: $0->$1, reason=[$2]",
        magic_enum::enum_name(traffic_class_.protocol), magic_enum::enum_name(protocol), reason);
    return false;
  }

  TrafficProtocol old_protocol = traffic_class_.protocol;
  traffic_class_.protocol = protocol;
  if (manager_ != nullptr) {
    manager_->UpdateProtocol(this, old_protocol);
  }
  CONN_TRACE(1) << absl::Substitute("Protocol changed: $0->$1", magic_enum::enum_name(old_protocol),
                                    magic_enum::enum_name(protocol));
  return true;
}

void ConnTracker::UpdateTimestamps(uint64_t bpf_timestamp) {
  last_bpf_timestamp_ns_ = std::max(last_bpf_timestamp_ns_, bpf_timestamp);

  last_update_timestamp_ = std::chrono::steady_clock::now();

  idle_iteration_ = false;
}

void ConnTracker::CheckTracker() {
  if (death_countdown_ >= 0 && death_countdown_ < kDeathCountdownIters - 1) {
    LOG_FIRST_N(WARNING, 10) << absl::Substitute(
        "Did not expect new event more than 1 sampling iteration after Close. Connection=$0.",
        ToString(conn_id_));
  }
}

DataStream* ConnTracker::req_data() {
  DCHECK_NE(traffic_class_.role, kRoleUnknown);
  switch (traffic_class_.role) {
    case kRoleClient:
      return &send_data_;
    case kRoleServer:
      return &recv_data_;
    default:
      return nullptr;
  }
}

DataStream* ConnTracker::resp_data() {
  DCHECK_NE(traffic_class_.role, kRoleUnknown);
  switch (traffic_class_.role) {
    case kRoleClient:
      return &recv_data_;
    case kRoleServer:
      return &send_data_;
    default:
      return nullptr;
  }
}

void ConnTracker::MarkForDeath(int32_t countdown) {
  DCHECK_GE(countdown, 0);

  if (death_countdown_ == -1) {
    CONN_TRACE(2) << absl::Substitute("Marked for death, countdown=$0", countdown);
  }

  // Only send the first time MarkForDeath is called (death_countdown == -1).
  if (death_countdown_ == -1 && ShouldExportToConnStats()) {
    conn_stats_->AddConnCloseEvent(*this);
  }

  // We received the close event.
  // Now give up to some more TransferData calls to receive trailing data events.
  // We do this for logging/debug purposes only.
  if (death_countdown_ >= 0) {
    death_countdown_ = std::min(death_countdown_, countdown);
  } else {
    death_countdown_ = countdown;
  }
}

bool ConnTracker::IsZombie() const { return death_countdown_ >= 0; }

bool ConnTracker::ReadyForDestruction() const {
  // We delay destruction time by a few iterations.
  // See also MarkForDeath().
  return death_countdown_ == 0;
}

bool ConnTracker::IsRemoteAddrInCluster(const std::vector<CIDRBlock>& cluster_cidrs) {
  PL_ASSIGN_OR(InetAddr remote_addr, open_info_.remote_addr.ToInetAddr(), return false);

  for (const auto& cluster_cidr : cluster_cidrs) {
    if (CIDRContainsIPAddr(cluster_cidr, remote_addr)) {
      return true;
    }
  }
  return false;
}

void ConnTracker::UpdateState(const std::vector<CIDRBlock>& cluster_cidrs) {
  // Don't handle anything but IP domain sockets.
  // Unspecified means we haven't figure out the SockAddrFamily yet, so let those through.
  if (open_info_.remote_addr.family != SockAddrFamily::kIPv4 &&
      open_info_.remote_addr.family != SockAddrFamily::kIPv6 &&
      open_info_.remote_addr.family != SockAddrFamily::kUnspecified) {
    Disable("Unhandled socket address family");
    return;
  }

  switch (traffic_class().role) {
    case EndpointRole::kRoleServer:
      if (state() == State::kCollecting) {
        state_ = State::kTransferring;
      }
      break;
    case EndpointRole::kRoleClient: {
      // Workaround: Server-side MySQL tracing seems to be busted, likely because of inference code.
      // TODO(oazizi/PL-1498): Remove this once service-side MySQL tracing is fixed.
      // TODO(oazizi): Remove DNS from this as well. Just keeping it in here for the demo,
      //               so we have more data in the tables.
      if (traffic_class().protocol == kProtocolMySQL || traffic_class().protocol == kProtocolDNS) {
        state_ = State::kTransferring;
        break;
      }

      if (cluster_cidrs.empty()) {
        CONN_TRACE(2) << "State not updated: MDS has not provided cluster CIDRs yet.";
        break;
      }

      if (conn_resolution_failed_) {
        Disable("No client-side tracing: Can't infer remote endpoint address.");
        break;
      }

      if (open_info_.remote_addr.family == SockAddrFamily::kUnspecified) {
        CONN_TRACE(2) << "State not updated: socket info resolution is still in progress.";
        // If resolution fails, then conn_resolution_failed_ will get set, and
        // this tracker should be disabled, see above.
        break;
      }

      // At this point we should only see IP addresses (not any other format).
      DCHECK(open_info_.remote_addr.family == SockAddrFamily::kIPv4 ||
             open_info_.remote_addr.family == SockAddrFamily::kIPv6);
      if (IsRemoteAddrInCluster(cluster_cidrs)) {
        Disable("No client-side tracing: Remote endpoint is inside the cluster.");
        break;
      }

      // Remote endpoint appears to be outside the cluster, so trace it.
      state_ = State::kTransferring;
    } break;
    case EndpointRole::kRoleUnknown:
      if (conn_resolution_failed_) {
        // TODO(yzhao): Incorporate parsing to detect message type, and back fill the role.
        // This is useful for Redis, for which eBPF protocol resolution cannot detect message type.
        Disable("Could not determine role for traffic because connection resolution failed.");
      } else if (conn_resolver_ == nullptr ||
                 last_update_timestamp_ != conn_resolver_->prev_infer_timestamp()) {
        CONN_TRACE(2)
            << "Protocol role was not inferred from BPF, waiting for user space inference result.";
      }
      break;
  }
}

void ConnTracker::UpdateDataStats(const SocketDataEvent& event) {
  switch (event.attr.direction) {
    case TrafficDirection::kEgress: {
      stats_.Increment(Stats::Key::kDataEventSent, 1);
      stats_.Increment(Stats::Key::kBytesSent, event.attr.msg_size);
      stats_.Increment(Stats::Key::kBytesSentTransferred, event.attr.msg_buf_size);
    } break;
    case TrafficDirection::kIngress: {
      stats_.Increment(Stats::Key::kDataEventRecv, 1);
      stats_.Increment(Stats::Key::kBytesRecv, event.attr.msg_size);
      stats_.Increment(Stats::Key::kBytesRecvTransferred, event.attr.msg_buf_size);
    } break;
  }
}

bool ConnTracker::ShouldExportToConnStats() const {
  if (conn_stats_ == nullptr) {
    return false;
  }

  bool endpoint_resolved = remote_endpoint().family == SockAddrFamily::kIPv4 ||
                           remote_endpoint().family == SockAddrFamily::kIPv6;
  bool role_resolved = (traffic_class_.role != kRoleUnknown);

  return endpoint_resolved && role_resolved;
}

void ConnTracker::ExportInitialConnStats() {
  conn_stats_->AddConnOpenEvent(*this);

  conn_stats_->RecordData(conn_id_.upid, traffic_class_, kEgress, remote_endpoint(),
                          stats_.Get(Stats::Key::kBytesSent));
  conn_stats_->RecordData(conn_id_.upid, traffic_class_, kIngress, remote_endpoint(),
                          stats_.Get(Stats::Key::kBytesRecv));
}

void ConnTracker::IterationPreTick(const std::vector<CIDRBlock>& cluster_cidrs,
                                   system::ProcParser* proc_parser,
                                   system::SocketInfoManager* socket_info_mgr) {
  // Assume no activity. This flag will be flipped if there is any activity during the iteration.
  idle_iteration_ = true;

  // Might not always be true, but for now there's nothing IterationPreTick does that
  // should be applied to a disabled tracker. This is in contrast to IterationPostTick.
  if (state_ == State::kDisabled) {
    return;
  }

  // If remote_addr is missing, it means the connect/accept was not traced.
  // Attempt to infer the connection information, to populate remote_addr.
  if (open_info_.remote_addr.family == SockAddrFamily::kUnspecified && socket_info_mgr != nullptr) {
    InferConnInfo(proc_parser, socket_info_mgr);

    // TODO(oazizi): If connection resolves to SockAddr type "Other",
    //               we should mark the state in BPF to Other too, so BPF stops tracing.
    //               We should also mark the ConnTracker for death.

    // If the address was successfully resolved, then send the connect information to conn stats.
    if (ShouldExportToConnStats()) {
      ExportInitialConnStats();
    }
  }

  UpdateState(cluster_cidrs);
}

void ConnTracker::IterationPostTick() {
  if (death_countdown_ > 0) {
    --death_countdown_;
    CONN_TRACE(2) << absl::Substitute("Death countdown=$0", death_countdown_);
  }

  HandleInactivity();

  // The rest of this function checks if if tracker should be disabled.

  // Nothing to do if it's already disabled.
  if (state() == State::kDisabled) {
    return;
  }

  if ((send_data().IsEOS() || recv_data().IsEOS())) {
    Disable("End-of-stream");
  }

  VLOG(1) << absl::Substitute(
      "$0 protocol=$1 state=$2 send_invalid_frames=$3 send_valid_frames=$4 send_raw_data_gaps=$5 "
      "recv_invalid_frames=$6 recv_valid_frames=$7, recv_raw_data_gaps=$8\n",
      ToString(conn_id_), magic_enum::enum_name(traffic_class().protocol),
      magic_enum::enum_name(state()), send_data().stat_invalid_frames(),
      send_data().stat_valid_frames(), send_data().stat_raw_data_gaps(),
      recv_data().stat_invalid_frames(), recv_data().stat_valid_frames(),
      recv_data().stat_raw_data_gaps());

  if ((send_data().ParseFailureRate() > kParseFailureRateThreshold) ||
      (recv_data().ParseFailureRate() > kParseFailureRateThreshold)) {
    Disable(absl::Substitute("Connection does not appear parseable as protocol $0",
                             magic_enum::enum_name(traffic_class().protocol)));
  }

  if (StitchFailureRate() > kStitchFailureRateThreshold) {
    Disable(absl::Substitute("Connection does not appear to produce valid records of protocol $0",
                             magic_enum::enum_name(traffic_class().protocol)));
  }
}

void ConnTracker::CheckProcForConnClose() {
  const auto& sysconfig = system::Config::GetInstance();
  std::filesystem::path fd_file = sysconfig.proc_path() / std::to_string(conn_id().upid.pid) /
                                  "fd" / std::to_string(conn_id().fd);

  if (!fs::Exists(fd_file).ok()) {
    CONN_TRACE(1) << "Socket file descriptor of the connection is closed. Marking for death";
    MarkForDeath(0);
  }
}

void ConnTracker::HandleInactivity() {
  idle_iteration_count_ = (idle_iteration_) ? idle_iteration_count_ + 1 : 0;

  // If the tracker is already marked for death, then no need to do anything.
  if (IsZombie()) {
    return;
  }

  // If the flag is set, we check proc more aggressively for close connections.
  // But only do this after a certain number of idle iterations.
  // Use exponential backoff on the idle_iterations to ensure we don't do this too frequently.
  if (FLAGS_stirling_check_proc_for_conn_close &&
      (idle_iteration_count_ >= idle_iteration_threshold_)) {
    CheckProcForConnClose();

    // Connection was idle, but not closed. Use exponential backoff for next idle check.
    // Max out the exponential backoff and go periodic once it's infrequent enough.
    // TODO(oazizi): Make kMinCheckPeriod related to sampling frequency.
    constexpr int kMinCheckPeriod = 100;
    idle_iteration_threshold_ += std::min(idle_iteration_threshold_, kMinCheckPeriod);
  }

  auto now = std::chrono::steady_clock::now();
  if (now > last_update_timestamp_ + InactivityDuration()) {
    // Flush the data buffers. Even if connection is still alive,
    // it is unlikely that any new data is a continuation of existing data in any meaningful way.
    Reset();

    // Reset timestamp so we don't enter this loop if statement again for some time.
    last_update_timestamp_ = now;
  }
}

double ConnTracker::StitchFailureRate() const {
  int total_attempts =
      stats_.Get(Stats::Key::kInvalidRecords) + stats_.Get(Stats::Key::kValidRecords);

  // Don't report rates until there some meaningful amount of events.
  // - Avoids division by zero.
  // - Avoids caller making decisions based on too little data.
  if (total_attempts <= 5) {
    return 0.0;
  }

  return 1.0 * stats_.Get(Stats::Key::kInvalidRecords) / total_attempts;
}

namespace {
Status ParseSocketInfoRemoteAddr(const system::SocketInfo& socket_info, SockAddr* addr) {
  switch (socket_info.family) {
    case AF_INET:
      PopulateInetAddr(std::get<struct in_addr>(socket_info.remote_addr), socket_info.remote_port,
                       addr);
      break;
    case AF_INET6:
      PopulateInet6Addr(std::get<struct in6_addr>(socket_info.remote_addr), socket_info.remote_port,
                        addr);
      break;
    case AF_UNIX:
      PopulateUnixAddr(std::get<struct un_path_t>(socket_info.remote_addr).path,
                       socket_info.remote_port, addr);
      break;
    default:
      return error::Internal("Unknown socket_info family: $0", socket_info.family);
  }

  return Status::OK();
}

EndpointRole TranslateRole(system::ClientServerRole role) {
  switch (role) {
    case system::ClientServerRole::kClient:
      return kRoleClient;
    case system::ClientServerRole::kServer:
      return kRoleServer;
    case system::ClientServerRole::kUnknown:
      return kRoleUnknown;
  }
  // Needed for GCC build.
  return {};
}

}  // namespace

void ConnTracker::InferConnInfo(system::ProcParser* proc_parser,
                                system::SocketInfoManager* socket_info_mgr) {
  DCHECK(proc_parser != nullptr);
  DCHECK(socket_info_mgr != nullptr);

  if (conn_resolution_failed_) {
    // We've previously tried and failed to perform connection inference,
    // so don't waste any time...a connection only gets one chance.
    CONN_TRACE(2) << "Skipping connection inference (previous inference attempt failed, and won't "
                     "try again).";
    return;
  }

  if (conn_resolver_ == nullptr) {
    conn_resolver_ = std::make_unique<FDResolver>(proc_parser, conn_id_.upid.pid, conn_id_.fd);
    bool success = conn_resolver_->Setup();
    if (!success) {
      conn_resolver_.reset();
      conn_resolution_failed_ = true;
      CONN_TRACE(2) << "Can't infer remote endpoint. Setup failed.";
    } else {
      CONN_TRACE(2) << "FDResolver has been created.";
    }
    // Return after Setup(), since we won't be able to infer until some time has elapsed.
    // This is because file descriptors can be re-used, and if we sample the FD just once,
    // we don't know which generation of the FD we've captured.
    // Waiting some time allows us to build some understanding of whether the FD is stable
    // at the time that we're interested at.
    return;
  }

  bool success = conn_resolver_->Update();
  if (!success) {
    conn_resolver_.reset();
    conn_resolution_failed_ = true;
    CONN_TRACE(2) << "Can't infer remote endpoint. Could not determine socket inode number of FD.";
    return;
  }

  // TODO(yzhao): Skip calling InferFDLink() if prev_infer_timestamp == infer_timestamp.
  auto prev_infer_timestamp = prev_sockinfo_infer_timestamp_;
  auto infer_timestamp = last_update_timestamp_;
  std::optional<std::string_view> fd_link_opt = conn_resolver_->InferFDLink(infer_timestamp);
  prev_sockinfo_infer_timestamp_ = infer_timestamp;
  if (!fd_link_opt.has_value()) {
    if (infer_timestamp != prev_infer_timestamp) {
      // Only trace if there has been new activities since last inference.
      CONN_TRACE(2) << "FD link info not available yet. Need more time determine the fd link and "
                       "resolve the connection.";
    }
    return;
  }

  // At this point we have something like "socket:[32431]" or "/dev/null" or "pipe:[2342]"
  // Next we extract the inode number, if it is a socket type.
  auto socket_inode_num_status = fs::ExtractInodeNum(fs::kSocketInodePrefix, fd_link_opt.value());

  // After resolving the FD, it may turn out to be non-socket data.
  // This is possible when the BPF read()/write() probes have inferred a protocol,
  // without seeing the connect()/accept().
  // For example, the captured data may have been for a FD to stdin/stdout, or a FD to a pipe.
  // Here we disable such non-socket connections.
  if (!socket_inode_num_status.ok()) {
    Disable("Resolved the connection to a non-socket type.");
    conn_resolver_.reset();
    return;
  }
  uint32_t socket_inode_num = socket_inode_num_status.ValueOrDie();

  // We found the inode number, now lets see if it maps to a known connection.
  StatusOr<const system::SocketInfo*> socket_info_status =
      socket_info_mgr->Lookup(conn_id().upid.pid, socket_inode_num);
  if (!socket_info_status.ok()) {
    conn_resolver_.reset();
    conn_resolution_failed_ = true;
    CONN_TRACE(2) << absl::Substitute("Could not map inode to a connection. Message = $0",
                                      socket_info_status.msg());
    return;
  }
  const system::SocketInfo& socket_info = *socket_info_status.ValueOrDie();

  // Success! Now copy the inferred socket information into the ConnTracker.

  Status s = ParseSocketInfoRemoteAddr(socket_info, &open_info_.remote_addr);
  if (!s.ok()) {
    conn_resolver_.reset();
    conn_resolution_failed_ = true;
    LOG(ERROR) << absl::Substitute("Remote address (type=$0) parsing failed. Message: $1",
                                   socket_info.family, s.msg());
    return;
  }

  EndpointRole inferred_role = TranslateRole(socket_info.role);
  SetRole(inferred_role, "inferred from socket info");

  CONN_TRACE(1) << absl::Substitute("Inferred connection dest=$0:$1",
                                    open_info_.remote_addr.AddrStr(),
                                    open_info_.remote_addr.port());

  // No need for the resolver anymore, so free its memory.
  conn_resolver_.reset();
}

}  // namespace stirling
}  // namespace pl