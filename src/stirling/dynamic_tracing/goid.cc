#include "src/stirling/dynamic_tracing/goid.h"

#include <filesystem>

#include "src/common/base/base.h"
#include "src/stirling/dynamic_tracing/dwarf_info.h"
#include "src/stirling/dynamic_tracing/ir/logical.pb.h"
#include "src/stirling/dynamic_tracing/ir/physical.pb.h"

namespace pl {
namespace stirling {
namespace dynamic_tracing {

ir::shared::Map GenGOIDMap() {
  ir::shared::Map map;
  map.set_name("pid_goid_map");
  return map;
}

// Returns an intermediate probe that traces creation of go routine.
ir::logical::Probe GenGOIDProbe() {
  ir::logical::Probe probe;

  // probe.trace_point.binary is not set. It's left for caller to attach to the same binary,
  // whose other probes need to access goid.

  probe.set_name("probe_entry_runtime_casgstatus");

  probe.mutable_trace_point()->set_symbol("runtime.casgstatus");
  probe.mutable_trace_point()->set_type(ir::shared::TracePoint::ENTRY);

  auto* constant = probe.add_consts();
  constant->set_name("kGRunningState");
  constant->set_type(ir::shared::ScalarType::INT64);
  // 2 indicates a new goid has been created, so it should be recorded in the map.
  constant->set_constant("2");

  auto* goid_arg = probe.add_args();
  goid_arg->set_id("goid_");
  goid_arg->set_expr("gp.goid");

  auto* newval_arg = probe.add_args();
  newval_arg->set_id("newval");
  newval_arg->set_expr("newval");

  auto* map_stash_action = probe.add_map_stash_actions();

  map_stash_action->set_map_name("pid_goid_map");
  map_stash_action->set_key(ir::shared::BPFHelper::TGID_PID);
  // TODO(yzhao): goid_ avoids conflict with the "goid" special variable.
  map_stash_action->add_value_variable_name("goid_");
  map_stash_action->mutable_cond()->set_op(ir::shared::Condition::EQUAL);
  map_stash_action->mutable_cond()->add_vars("newval");
  map_stash_action->mutable_cond()->add_vars("kGRunningState");

  return probe;
}

}  // namespace dynamic_tracing
}  // namespace stirling
}  // namespace pl