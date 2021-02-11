#pragma once

#ifndef __linux__

#include "src/stirling/core/source_connector.h"

namespace pl {
namespace stirling {

DUMMY_SOURCE_CONNECTOR(PIDRuntimeConnector);

}  // namespace stirling
}  // namespace pl
#else

#include <map>
#include <memory>
#include <string>
#include <utility>
#include <vector>

#include "src/common/base/base.h"
#include "src/stirling/bpf_tools/bcc_wrapper.h"
#include "src/stirling/core/canonical_types.h"
#include "src/stirling/core/source_connector.h"
#include "src/stirling/source_connectors/pid_runtime/bcc_bpf_intf/pidruntime.h"

namespace pl {
namespace stirling {

class PIDRuntimeConnector : public SourceConnector, public bpf_tools::BCCWrapper {
 public:
  // clang-format off
  static constexpr DataElement kElements[] = {
      canonical_data_elements::kTime,
      // TODO(yzhao): Change to upid.
      {"pid", "Process PID",
       types::DataType::INT64, types::SemanticType::ST_NONE, types::PatternType::GENERAL},
      // TODO(chengruizhe): Convert to counter.
      {"runtime_ns", "Process runtime in nanoseconds",
       types::DataType::INT64, types::SemanticType::ST_NONE, types::PatternType::METRIC_GAUGE},
      {"cmd", "Process command line",
       types::DataType::STRING, types::SemanticType::ST_NONE, types::PatternType::GENERAL},
  };
  // clang-format on

  static constexpr auto kTable =
      DataTableSchema("bcc_pid_cpu_usage", "CPU usage metrics for processes (obtained via BPF)",
                      kElements, std::chrono::milliseconds{100}, std::chrono::milliseconds{1000});

  static constexpr auto kTables = MakeArray(kTable);

  static std::unique_ptr<SourceConnector> Create(std::string_view name) {
    return std::unique_ptr<SourceConnector>(new PIDRuntimeConnector(name));
  }

  Status InitImpl() override;
  Status StopImpl() override;
  void TransferDataImpl(ConnectorContext* ctx, uint32_t table_num, DataTable* data_table) override;

 protected:
  explicit PIDRuntimeConnector(std::string_view name)
      : SourceConnector(name, kTables), bpf_tools::BCCWrapper() {}

 private:
  // Freq. (in Hz) at which to trigger bpf func.
  static constexpr uint64_t kSamplingFreqHz = 99;
  static constexpr auto kSamplingProbes =
      MakeArray<bpf_tools::SamplingProbeSpec>({"trace_pid_runtime", kSamplingFreqHz});

  std::map<uint16_t, uint64_t> prev_run_time_map_;
};

}  // namespace stirling
}  // namespace pl

#endif