#include "src/vizier/services/agent/kelvin/kelvin_manager.h"

#include "src/vizier/services/agent/manager/exec.h"
#include "src/vizier/services/agent/manager/manager.h"

namespace pl {
namespace vizier {
namespace agent {

Status KelvinManager::InitImpl() { return Status::OK(); }

Status KelvinManager::PostRegisterHook() {
  auto execute_query_handler = std::make_shared<ExecuteQueryMessageHandler>(
      dispatcher_.get(), info(), nats_connector(), carnot_.get());
  PL_RETURN_IF_ERROR(RegisterMessageHandler(messages::VizierMessage::MsgCase::kExecuteQueryRequest,
                                            execute_query_handler));

  return Status::OK();
}

Status KelvinManager::StopImpl(std::chrono::milliseconds) { return Status::OK(); }

}  // namespace agent
}  // namespace vizier
}  // namespace pl