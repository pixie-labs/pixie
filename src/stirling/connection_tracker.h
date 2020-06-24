#pragma once

#include <any>
#include <deque>
#include <map>
#include <memory>
#include <string>
#include <utility>
#include <variant>
#include <vector>

#include "src/common/system/proc_parser.h"
#include "src/common/system/socket_info.h"
#include "src/stirling/bcc_bpf_interface/go_grpc_types.h"
#include "src/stirling/common/go_grpc_types.h"
#include "src/stirling/common/protocol_traits.h"
#include "src/stirling/common/socket_trace.h"
#include "src/stirling/cql/cql_stitcher.h"
#include "src/stirling/data_stream.h"
#include "src/stirling/fd_resolver.h"
#include "src/stirling/http/http_stitcher.h"
#include "src/stirling/http2u/stitcher.h"
#include "src/stirling/mysql/mysql_parse.h"
#include "src/stirling/mysql/mysql_stitcher.h"
#include "src/stirling/pgsql/parse.h"
#include "src/stirling/socket_trace_bpf_tables.h"

DECLARE_bool(enable_unix_domain_sockets);

#define CONN_TRACE(level) \
  LOG_IF(INFO, level <= debug_trace_level_) << absl::Substitute("$0 ", ToString(conn_id_))

namespace pl {
namespace stirling {

// Forward declaration to avoid circular include of connection_stats.h and connection_tracker.h.
class ConnectionStats;

/**
 * @brief Describes a connection from user space. This corresponds to struct conn_info_t in
 * src/stirling/bcc_bpf_interface/socket_trace.h.
 */
struct SocketOpen {
  uint64_t timestamp_ns = 0;
  // TODO(yzhao): Consider using std::optional to indicate the address has not been initialized.
  SockAddr remote_addr;
};

struct SocketClose {
  uint64_t timestamp_ns = 0;
  // The send/write sequence number at time of close.
  uint64_t send_seq_num = 0;
  // The recv/read sequence number at time of close.
  uint64_t recv_seq_num = 0;
};

/**
 * Connection tracker is the main class that tracks all the events for a monitored TCP connection.
 *
 * It collects the connection info (e.g. remote IP, port),
 * and all the send/recv data observed on the connection.
 *
 * Data is extracted from a connection tracker and pushed out, as the data becomes parseable.
 */
class ConnectionTracker {
 public:
  // State values change monotonically from lower to higher values; and cannot change reversely.
  enum class State {
    // When collecting, the tracker collects data from BPF, but does not push them to table store.
    kCollecting,

    // When transferring, the tracker pushes data to table store.
    kTransferring,

    // When disabled, the tracker will silently drop existing data and silently drop any new data.
    // It will, however, still track open and close events.
    kDisabled,
  };

  ConnectionTracker() = default;
  ConnectionTracker(ConnectionTracker&& other) = default;

  ~ConnectionTracker();

  void set_conn_stats(ConnectionStats* conn_stats) { conn_stats_ = conn_stats; }

  /**
   * @brief Registers a BPF connection control event into the tracker.
   *
   * @param event The data event from BPF.
   */
  void AddControlEvent(const socket_control_event_t& event);

  /**
   * @brief Registers a BPF data event into the tracker.
   *
   * @param event The data event from BPF.
   */
  void AddDataEvent(std::unique_ptr<SocketDataEvent> event);

  /**
   * Add a recorded HTTP2 header (name-value pair).
   * The struct should contain stream ID and other meta-data so it can matched with other HTTP2
   * header events and data frames.
   *
   * @param data The event from BPF uprobe.
   */
  void AddHTTP2Header(std::unique_ptr<HTTP2HeaderEvent> data);

  /**
   * Add a recorded HTTP2 data frame.
   * The struct should contain stream ID and other meta-data so it can matched with other HTTP2
   * header events and data frames.
   *
   * @param data The event from BPF uprobe.
   */
  void AddHTTP2Data(std::unique_ptr<HTTP2DataEvent> data);

  /**
   * @brief Attempts to infer the remote endpoint of a connection.
   *
   * Intended for cases where the accept/connect was not traced.
   *
   * @param proc_parser Pointer to a proc_parser for access to /proc filesystem.
   * @param connections A map of inodes to endpoint information.
   */
  void InferConnInfo(system::ProcParser* proc_parser, system::SocketInfoManager* socket_info_mgr);

  /**
   * @brief Processes the connection tracker, parsing raw events into frames,
   * and frames into record.
   *
   * @tparam TRecordType the type of the entries to be parsed.
   * @return Vector of processed entries.
   */
  template <typename TProtocolTraits>
  std::vector<typename TProtocolTraits::record_type> ProcessToRecords() {
    using TRecordType = typename TProtocolTraits::record_type;
    using TFrameType = typename TProtocolTraits::frame_type;
    using TStateType = typename TProtocolTraits::state_type;

    DataStreamsToFrames<TFrameType>();

    InitProtocolState<TStateType>();

    auto& req_frames = req_data()->Frames<TFrameType>();
    auto& resp_frames = resp_data()->Frames<TFrameType>();
    auto state_ptr = protocol_state<TStateType>();

    CONN_TRACE(1) << absl::Substitute("req_frames=$0 resp_frames=$1", req_frames.size(),
                                      resp_frames.size());

    RecordsWithErrorCount<TRecordType> result = ProcessFrames(&req_frames, &resp_frames, state_ptr);

    CONN_TRACE(1) << absl::Substitute("records=$0", result.records.size());

    stat_invalid_records_ += result.error_count;
    stat_valid_records_ += result.records.size();

    Cleanup<TProtocolTraits>();

    return result.records;
  }

  /**
   * @brief Returns reference to current set of unconsumed requests.
   * Note: A call to ProcessBytesToFrames() is required to parse new requests.
   */
  template <typename TFrameType>
  std::deque<TFrameType>& req_frames() {
    return req_data()->Frames<TFrameType>();
  }
  // TODO(yzhao): req_data() requires traffic_class_.role to be set. But HTTP2 uprobe tracing does
  // not set that. So send_data() is created. Investigate more unified approach.
  template <typename TFrameType>
  const std::deque<TFrameType>& send_frames() const {
    return send_data_.Frames<TFrameType>();
  }

  const std::deque<http2u::Stream>& http2_send_streams() const {
    return send_data_.http2_streams();
  }
  const std::deque<http2u::Stream>& http2_recv_streams() const {
    return recv_data_.http2_streams();
  }

  /**
   * @brief Returns reference to current set of unconsumed responses.
   * Note: A call to ProcessBytesToFrames() is required to parse new responses.
   */
  template <typename TFrameType>
  std::deque<TFrameType>& resp_frames() {
    return resp_data()->Frames<TFrameType>();
  }
  template <typename TFrameType>
  const std::deque<TFrameType>& recv_frames() const {
    return recv_data_.Frames<TFrameType>();
  }

  const conn_id_t& conn_id() const { return conn_id_; }
  const traffic_class_t& traffic_class() const { return traffic_class_; }

  /**
   * Get remote IP endpoint of the connection.
   *
   * @return IP.
   */
  const SockAddr& remote_endpoint() const { return open_info_.remote_addr; }

  /**
   * @brief Get the connection information (e.g. remote IP, port, PID, etc.) for this connection.
   *
   * @return connection information.
   */
  const SocketOpen& conn() const { return open_info_; }

  /**
   * @brief Get the DataStream of sent frames for this connection.
   *
   * @return Data stream of send data.
   */
  const DataStream& send_data() const { return send_data_; }

  /**
   * @brief Get the DataStream of received frames for this connection.
   *
   * @return Data stream of received data.
   */
  const DataStream& recv_data() const { return recv_data_; }

  /**
   * @brief Get the DataStream of requests for this connection.
   *
   * @return Data stream of requests.
   */
  DataStream* req_data();

  /**
   * @brief Get the DataStream of responses for this connection.
   *
   * @return Data stream of responses.
   */
  DataStream* resp_data();

  /**
   * @return Returns the latest timestamp of all BPF events received by this tracker (using BPF
   * timestamp).
   */
  uint64_t last_bpf_timestamp_ns() { return last_bpf_timestamp_ns_; }

  /**
   * @return Returns the a timestamp the last time an event was added to this tracker (using
   * steady_clock).
   */
  std::chrono::time_point<std::chrono::steady_clock> last_update_timestamp() {
    return last_update_timestamp_;
  }

  /**
   * Resets the state of the connection tracker, clearing all data and state.
   */
  void Reset();

  /**
   * @brief Disables the connection tracker. The tracker will drop all its existing data,
   * and also not accept any future data (future data events will be ignored).
   *
   * The tracker will still wait for a Close event to get destroyed.
   */
  void Disable(std::string_view reason = "");

  /**
   * If disabled, returns the reason the tracker was disabled.
   */
  std::string_view disable_reason() const { return std::string_view(disable_reason_); }

  /**
   * Returns a state that determine the operations performed on the traffic traced on the
   * connection.
   */
  State state() const { return state_; }

  /**
   * @brief Check if all events have been received on this stream.
   * Implies that the Close() event has been received as well.
   *
   * @return whether all data events and connection close have been received.
   */
  bool AllEventsReceived() const;

  /**
   * @brief Marks the ConnectionTracker for death.
   *
   * This indicates that the tracker should not receive any further events,
   * otherwise an warning or error will be produced.
   */
  void MarkForDeath(int32_t countdown = kDeathCountdownIters);

  /**
   * @brief Returns true if this tracker has been marked for death.
   *
   * @return true if this tracker is on its death countdown.
   */
  bool IsZombie() const;

  /**
   * @brief Whether this ConnectionTracker can be destroyed.
   * @return true if this ConnectionTracker is a candidate for destruction.
   */
  bool ReadyForDestruction() const;

  /**
   * @brief Updates the any state that changes per iteration on this connection tracker.
   * Should be called once per sampling, after ProcessToRecords().
   */
  void IterationPostTick();

  /**
   * @brief Performs any preprocessing that should happen per iteration on this
   * connection tracker.
   * Should be called once per sampling, before ProcessToRecords().
   *
   * @param proc_parser Pointer to a proc_parser for access to /proc filesystem.
   * @param connections A map of inodes to endpoint information.
   */
  void IterationPreTick(const std::vector<CIDRBlock>& cluster_cidrs,
                        system::ProcParser* proc_parser,
                        system::SocketInfoManager* socket_info_mgr);

  /**
   * @brief Sets a the duration after which a connection is deemed to be inactive.
   * After becoming inactive, the connection may either (1) have its buffers purged,
   * where any unparsed frames are discarded or (2) be removed entirely from the
   * set of tracked connections. The main difference between (1) and (2) are that
   * in (1) some connection information is retained in case the connection becomes
   * active again.
   *
   * NOTE: This function is static, because it is currently only intended to be
   * used for testing purposes. If ever a need arises to have different inactivity
   * durations per connection tracker, then this function (and related functions below)
   * should be made into a member function.
   *
   * @param duration The duration in seconds, with no events, after which a connection
   * is deemed to be inactive.
   */
  static void SetInactivityDuration(std::chrono::seconds duration) {
    inactivity_duration_ = duration;
  }
  static constexpr std::chrono::seconds kDefaultInactivityDuration{300};

  /**
   * @brief Return the currently configured duration, after which a connection is deemed to be
   * inactive.
   */
  static std::chrono::seconds InactivityDuration() { return inactivity_duration_; }

  /**
   * Fraction of frame stitching attempts that resulted in an invalid record.
   */
  double StitchFailureRate() const;

  enum class CountStats {
    // The number of sent/received data events.
    kDataEventSent = 0,
    kDataEventRecv,

    // The number of sent/received bytes.
    kBytesSent,
    kBytesRecv,
  };

  /**
   * Increment a stats event counter for this tracker.
   * @param stat stat selector.
   */
  void IncrementStat(CountStats stat, int count = 1) { stats_[static_cast<int>(stat)] += count; }

  /**
   * Get current value of a stats event counter for this tracker.
   * @param stat stat selector.
   * @return stat count value.
   */
  uint64_t Stat(CountStats stat) const { return stats_[static_cast<int>(stat)]; }

  /**
   * @brief Number of TransferData() (i.e. PerfBuffer read) calls during which a ConnectionTracker
   * persists after it has been marked for death. We keep ConnectionTrackers alive to catch
   * late-arriving events, and for debug purposes.
   *
   * Note that an event may arrive appear to up to 1 iteration late.
   * This is caused by the order we read the perf buffers.   *
   * Example where event appears to arrive late:
   *  T0 - read perf buffer of data events
   *  T1 - DataEvent recorded
   *  T2 - CloseEvent recorded
   *  T3 - read perf buffer of close events <---- CloseEvent observed here
   *  ...
   *  T4 - read perf buffer of data events <---- DataEvent observed here
   * In such cases, the timestamps still show the DataEvent as occurring first.
   */
  static constexpr int64_t kDeathCountdownIters = 3;

  /**
   * Initializes protocol state for a protocol.
   */
  template <typename TStateType>
  void InitProtocolState() {
    // A protocol can specify that it has no state by setting ProtocolTraits::state_type to
    // NoState.
    // As an optimization, we don't call std::make_unique in such cases.
    // No need to create an object on the heap for protocols that don't have state.
    // Note that protocol_state() has the same `if constexpr`, for this optimization to work.
    if constexpr (!std::is_same_v<TStateType, NoState>) {
      TStateType* state_types_ptr = std::any_cast<TStateType>(&protocol_state_);
      if (state_types_ptr == nullptr) {
        protocol_state_.emplace<TStateType>();
      }
    }
  }

  /**
   * Returns the current protocol state for a protocol.
   */
  template <typename TStateType>
  TStateType* protocol_state() {
    // See note in InitProtocolState about this `if constexpr`.
    if constexpr (std::is_same_v<TStateType, NoState>) {
      return nullptr;
    } else {
      TStateType* ptr = std::any_cast<TStateType>(&protocol_state_);
      return ptr;
    }
  }

  template <typename TProtocolTraits>
  void Cleanup() {
    using TFrameType = typename TProtocolTraits::frame_type;
    using TStateType = typename TProtocolTraits::state_type;

    if constexpr (std::is_same_v<TFrameType, http2u::Stream>) {
      send_data_.CleanupHTTP2Streams();
      recv_data_.CleanupHTTP2Streams();
    } else {
      send_data_.CleanupFrames<TFrameType>();
      recv_data_.CleanupFrames<TFrameType>();
    }

    if (send_data_.CleanupEvents()) {
      protocol_state<TStateType>()->global = {};
      protocol_state<TStateType>()->send = {};
    }
    if (recv_data_.CleanupEvents()) {
      protocol_state<TStateType>()->global = {};
      protocol_state<TStateType>()->recv = {};
    }

    // TODO(yzhao): For http2, it's likely the case that we'll want to preserve the inflater under
    // situations where the HEADERS frames have not been lost. Detecting and responding to them
    // probably will change the semantic of Reset(), such that it will means different thing for
    // different protocols.
  }

  static void SetConnInfoMapManager(const std::shared_ptr<ConnInfoMapManager>& conn_info_map_mgr) {
    conn_info_map_mgr_ = conn_info_map_mgr;
  }

  void SetDebugTrace(int level) { debug_trace_level_ = level; }
  void SetTrafficClass(struct traffic_class_t traffic_class);

 private:
  void AddConnOpenEvent(const conn_event_t& conn_info);
  void AddConnCloseEvent(const close_event_t& close_event);

  void SetConnID(struct conn_id_t conn_id);
  void UpdateTimestamps(uint64_t bpf_timestamp);
  void CheckTracker();
  void HandleInactivity();
  bool IsRemoteAddrInCluster(const std::vector<CIDRBlock>& cluster_cidrs);
  void UpdateState(const std::vector<CIDRBlock>& cluster_cidrs);

  void UpdateDataStats(const SocketDataEvent& event);
  bool ReadyToExportDataStats() const {
    return remote_endpoint().family == SockAddrFamily::kIPv4 ||
           remote_endpoint().family == SockAddrFamily::kIPv6 || conn_resolution_failed_;
  }
  void ExportDataStats();
  void ExportConnCloseStats();

  // conn_info_map_mgr_ is used to release BPF map resources when a ConnectionTracker is destroyed.
  // It is a safety net, since BPF should release the resources as long as the close() syscall is
  // made. Note that since there is only one global BPF map, this is a static/global structure.
  static inline std::shared_ptr<ConnInfoMapManager> conn_info_map_mgr_;

  template <typename TFrameType>
  void DataStreamsToFrames() {
    DataStream* resp_data_ptr = resp_data();
    DCHECK(resp_data_ptr != nullptr);
    resp_data_ptr->template ProcessBytesToFrames<TFrameType>(MessageType::kResponse);

    DataStream* req_data_ptr = req_data();
    DCHECK(req_data_ptr != nullptr);
    req_data_ptr->template ProcessBytesToFrames<TFrameType>(MessageType::kRequest);
  }

  int debug_trace_level_ = 0;

  // Used to identify the remove endpoint in case the accept/connect was not traced.
  std::unique_ptr<FDResolver> conn_resolver_ = nullptr;
  bool conn_resolution_failed_ = false;

  struct conn_id_t conn_id_ = {};
  struct traffic_class_t traffic_class_ {
    kProtocolUnknown, kRoleNone
  };
  SocketOpen open_info_;
  SocketClose close_info_;

  // The data collected by the stream, one per direction.
  DataStream send_data_;
  DataStream recv_data_;

  // --- Start uprobe-based HTTP2 members.

  // Uprobe-based HTTP2 uses a different scheme, where it holds client and server-initiated streams,
  // instead of send and recv messages. As such, we create aliases for HTTP2.
  DataStream& client_streams_ = send_data_;
  DataStream& server_streams_ = recv_data_;

  // For uprobe-based HTTP2 tracing only.
  // Tracks oldest active stream ID for retiring the head of send_data_/recv_data_ deques.
  uint32_t oldest_active_client_stream_id_;
  uint32_t oldest_active_server_stream_id_;

  // Access the appropriate HalfStream object for the given stream ID.
  http2u::HalfStream* HalfStreamPtr(uint32_t stream_id, bool write_event);

  // According to the HTTP2 protocol, Stream IDs are incremented by 2.
  // Client-initiated streams use odd IDs, while server-initiated streams use even IDs.
  static constexpr int kHTTP2StreamIDIncrement = 2;

  // --- End uprobe-based HTTP2 members.

  // The timestamp of the last activity on this connection.
  // Recorded as the latest timestamp on a BPF event.
  uint64_t last_bpf_timestamp_ns_ = 0;

  // The timestamp of the last update on this connection which alters the states.
  //
  // Recorded as the latest touch time on the ConnectionTracker.
  // Currently using steady clock, so cannot be used meaningfully for logging real times.
  // This can be changed in the future if required.
  std::chrono::time_point<std::chrono::steady_clock> last_update_timestamp_;

  State state_ = State::kCollecting;

  std::string disable_reason_;

  inline static std::chrono::seconds inactivity_duration_ = kDefaultInactivityDuration;

  // Iterations before the tracker can be killed.
  int32_t death_countdown_ = -1;

  // Keep some stats on ProcessFrames() calls.
  int stat_invalid_records_ = 0;
  int stat_valid_records_ = 0;

  ConnectionStats* conn_stats_ = nullptr;
  bool data_stats_exported_ = false;
  std::vector<uint64_t> stats_ = std::vector<uint64_t>(magic_enum::enum_count<CountStats>());

  // Connection trackers need to keep a state because there can be information between
  // needed from previous requests/responses needed to parse or render current request.
  // E.g. MySQL keeps a map of previously occurred stmt prepare events as the state such
  // that future stmt execute events can match up with the correct one using stmt_id.
  //
  // TODO(oazizi): Is there a better structure than std::any?
  // One alternative is an std::variant, but that becomes tedious to maintain with a
  // growing number of protocols.
  // Two considerations:
  // 1) We want something with an interface-type API. The current structure does achieve
  //    this, but not in a clear way. The compilation errors will be due to SFINAE and
  //    hard to interpret.
  // 2) We want something with some type safety. std::any does provide this, in the
  //    similar way as std::variant.
  std::any protocol_state_;

  template <typename TProtocolTraits>
  friend std::string DebugString(const ConnectionTracker& c, std::string_view prefix);
};

// Explicit template specialization must be declared in namespace scope.
// See https://en.cppreference.com/w/cpp/language/member_template
template <>
std::vector<http2u::Record> ConnectionTracker::ProcessToRecords<http2u::ProtocolTraits>();

template <typename TProtocolTraits>
std::string DebugString(const ConnectionTracker& c, std::string_view prefix) {
  using TFrameType = typename TProtocolTraits::frame_type;

  std::string info;
  info += absl::Substitute("$0conn_id=$1\n", prefix, ToString(c.conn_id()));
  info += absl::Substitute("state=$0\n", magic_enum::enum_name(c.state()));
  info += absl::Substitute("$0remote_addr=$1:$2\n", prefix, c.remote_endpoint().AddrStr(),
                           c.remote_endpoint().port);
  info += absl::Substitute("$0protocol=$1\n", prefix,
                           magic_enum::enum_name(c.traffic_class().protocol));
  info += absl::Substitute("$0recv queue\n", prefix);
  info += DebugString<TFrameType>(c.recv_data(), absl::StrCat(prefix, "  "));
  info += absl::Substitute("$0send queue\n", prefix);
  info += DebugString<TFrameType>(c.send_data(), absl::StrCat(prefix, "  "));
  return info;
}

}  // namespace stirling
}  // namespace pl