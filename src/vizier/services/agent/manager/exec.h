#pragma once

#include <memory>

#include <absl/container/flat_hash_map.h>
#include "src/carnot/plan/plan.h"
#include "src/vizier/services/agent/manager/manager.h"
#include "src/vizier/services/query_broker/querybrokerpb/service.grpc.pb.h"

namespace pl {
namespace vizier {
namespace agent {

/**
 * ExecuteQueryMessageHandler takes execute query results and performs them.
 * If a qb_stub is specified the results will also be RPCd to the query broker,
 * otherwise only query execution is performed.
 *
 * This class runs all of it's work on a thread pool and tracks pending queries internally.
 */
class ExecuteQueryMessageHandler : public Manager::MessageHandler {
 public:
  using QueryBrokerService = pl::vizier::services::query_broker::querybrokerpb::QueryBrokerService;
  using QueryBrokerServiceSPtr = std::shared_ptr<QueryBrokerService::Stub>;

  ExecuteQueryMessageHandler() = delete;
  ExecuteQueryMessageHandler(pl::event::Dispatcher* dispatcher, Info* agent_info,
                             Manager::VizierNATSConnector* nats_conn,
                             QueryBrokerServiceSPtr qb_stub, carnot::Carnot* carnot);
  ~ExecuteQueryMessageHandler() override = default;

  Status HandleMessage(std::unique_ptr<messages::VizierMessage> msg) override;

 protected:
  /**
   * HandleQueryExecutionComplete can be called by the async task to signal that work has been
   * completed.
   */
  virtual void HandleQueryExecutionComplete(sole::uuid query_id);

 private:
  // Forward declare private task class.
  class ExecuteQueryTask;

  QueryBrokerServiceSPtr qb_stub_;
  carnot::Carnot* carnot_;

  // Map from query_id -> Running query task.
  absl::flat_hash_map<sole::uuid, pl::event::RunnableAsyncTaskUPtr> running_queries_;
};

// TODO(nserrino): Delete this function when the batch API is deprecated.
inline StatusOr<bool> PlanContainsBatchResults(const carnot::planpb::Plan& plan_pb) {
  auto no_op = [&](const auto&) { return Status::OK(); };
  carnot::plan::Plan plan;
  PL_RETURN_IF_ERROR(plan.Init(plan_pb));

  bool plan_contains_batch_results = false;

  auto s = carnot::plan::PlanWalker()
               .OnPlanFragment([&](auto* pf) {
                 return carnot::plan::PlanFragmentWalker()
                     .OnMemorySink([&](auto&) {
                       plan_contains_batch_results = true;
                       return Status::OK();
                     })
                     .OnMap(no_op)
                     .OnAggregate(no_op)
                     .OnFilter(no_op)
                     .OnLimit(no_op)
                     .OnMemorySource(no_op)
                     .OnUnion(no_op)
                     .OnJoin(no_op)
                     .OnGRPCSource(no_op)
                     .OnGRPCSink(no_op)
                     .OnUDTFSource(no_op)
                     .Walk(pf);
               })
               .Walk(&plan);

  PL_RETURN_IF_ERROR(s);
  return plan_contains_batch_results;
}

}  // namespace agent
}  // namespace vizier
}  // namespace pl