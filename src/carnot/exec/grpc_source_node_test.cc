#include "src/carnot/exec/grpc_source_node.h"

#include <utility>
#include <vector>

#include <gtest/gtest.h>
#include <sole.hpp>

#include "src/carnot/exec/test_utils.h"
#include "src/carnot/planpb/test_proto.h"
#include "src/carnot/udf/registry.h"
#include "src/common/base/base.h"
#include "src/common/testing/testing.h"
#include "src/shared/types/types.h"

namespace pl {
namespace carnot {
namespace exec {

using table_store::schema::RowBatch;
using table_store::schema::RowDescriptor;
using ::testing::_;

class GRPCSourceNodeTest : public ::testing::Test {
 public:
  GRPCSourceNodeTest() {
    func_registry_ = std::make_unique<udf::Registry>("test_registry");
    auto table_store = std::make_shared<table_store::TableStore>();

    exec_state_ = std::make_unique<ExecState>(func_registry_.get(), table_store,
                                              MockKelvinStubGenerator, sole::uuid4());

    table_store::schema::Relation rel({types::DataType::BOOLEAN, types::DataType::TIME64NS},
                                      {"col1", "time_"});
  }

 protected:
  std::unique_ptr<ExecState> exec_state_;
  std::unique_ptr<udf::Registry> func_registry_;
};

TEST_F(GRPCSourceNodeTest, basic) {
  auto op_proto = planpb::testutils::CreateTestGRPCSource1PB();
  std::unique_ptr<plan::Operator> plan_node = plan::GRPCSourceOperator::FromProto(op_proto, 1);
  RowDescriptor output_rd({types::DataType::INT64});

  auto tester = exec::ExecNodeTester<GRPCSourceNode, plan::GRPCSourceOperator>(
      *plan_node, output_rd, std::vector<RowDescriptor>({}), exec_state_.get());

  for (auto i = 0; i < 3; ++i) {
    EXPECT_TRUE(tester.node()->HasBatchesRemaining());
    EXPECT_FALSE(tester.node()->NextBatchReady());

    std::vector<types::Int64Value> data(i, i);
    auto rb = RowBatchBuilder(output_rd, i, /*eow*/ i == 2, /*eos*/ i == 2)
                  .AddColumn<types::Int64Value>(data)
                  .get();

    auto rb_wrapper = std::make_unique<carnotpb::RowBatchRequest>();
    EXPECT_OK(rb.ToProto(rb_wrapper->mutable_row_batch()));
    EXPECT_TRUE(tester.node()->EnqueueRowBatch(std::move(rb_wrapper)).ok());

    EXPECT_TRUE(tester.node()->NextBatchReady());
    tester.GenerateNextResult().ExpectRowBatch(rb);
  }

  EXPECT_FALSE(tester.node()->NextBatchReady());
  EXPECT_FALSE(tester.node()->HasBatchesRemaining());
}

}  // namespace exec
}  // namespace carnot
}  // namespace pl