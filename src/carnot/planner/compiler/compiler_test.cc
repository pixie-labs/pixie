#include <gmock/gmock.h>
#include <google/protobuf/text_format.h>
#include <google/protobuf/util/message_differencer.h>
#include <gtest/gtest.h>

#include <tuple>
#include <unordered_map>
#include <vector>

#include <pypa/parser/parser.hh>

#include "src/carnot/funcs/funcs.h"
#include "src/carnot/planner/compiler/compiler.h"
#include "src/carnot/planner/compiler/test_utils.h"
#include "src/carnot/planpb/plan.pb.h"
#include "src/carnot/planpb/test_proto.h"
#include "src/carnot/udf_exporter/udf_exporter.h"
#include "src/common/testing/testing.h"

namespace pl {
namespace carnot {
namespace planner {
namespace compiler {

using ::pl::table_store::schema::Relation;
using ::pl::testing::proto::EqualsProto;
using planpb::testutils::CompareLogicalPlans;
using ::testing::_;
using ::testing::ContainsRegex;
using ::testing::ElementsAre;

constexpr char kExtraScalarUDFs[] = R"proto(
scalar_udfs {
  name: "equal"
  exec_arg_types: UINT128
  exec_arg_types: UINT128
  return_type: BOOLEAN
}
)proto";
class CompilerTest : public ::testing::Test {
 protected:
  void SetUpRegistryInfo() {
    // TODO(philkuz) replace the following call info_
    // info_ = udfexporter::ExportUDFInfo().ConsumeValueOrDie();
    auto func_registry = std::make_unique<udf::Registry>("func_registry");
    funcs::RegisterFuncsOrDie(func_registry.get());
    auto udf_proto = func_registry->ToProto();

    std::string new_udf_info = absl::Substitute("$0$1", udf_proto.DebugString(), kExtraScalarUDFs);
    google::protobuf::TextFormat::MergeFromString(new_udf_info, &udf_proto);

    info_ = std::make_unique<planner::RegistryInfo>();
    PL_CHECK_OK(info_->Init(udf_proto));
  }

  void SetUp() override {
    SetUpRegistryInfo();

    auto rel_map = std::make_unique<RelationMap>();
    rel_map->emplace("sequences", Relation(
                                      {
                                          types::TIME64NS,
                                          types::FLOAT64,
                                          types::FLOAT64,
                                      },
                                      {"time_", "xmod10", "PIx"}));

    rel_map->emplace("cpu", Relation({types::INT64, types::FLOAT64, types::FLOAT64, types::FLOAT64,
                                      types::UINT128},
                                     {"count", "cpu0", "cpu1", "cpu2", "upid"}));
    cgroups_relation_ =
        Relation({types::TIME64NS, types::STRING, types::STRING}, {"time_", "qos", "_attr_pod_id"});

    rel_map->emplace("cgroups", cgroups_relation_);

    rel_map->emplace("http_table",
                     Relation({types::TIME64NS, types::UINT128, types::INT64, types::INT64},

                              {"time_", "upid", "http_resp_status", "http_resp_latency_ns"}));
    rel_map->emplace("network", Relation({types::UINT128, types::INT64, types::INT64, types::INT64},
                                         {MetadataProperty::kUniquePIDColumn, "bytes_in",
                                          "bytes_out", "agent_id"}));
    rel_map->emplace("process_stats",
                     Relation({types::TIME64NS, types::UINT128, types::INT64, types::INT64,
                               types::INT64, types::INT64, types::INT64, types::INT64, types::INT64,
                               types::INT64, types::INT64, types::INT64, types::INT64},
                              {"time_", "upid", "major_faults", "minor_faults", "cpu_utime_ns",
                               "cpu_ktime_ns", "num_threads", "vsize_bytes", "rss_bytes",
                               "rchar_bytes", "wchar_bytes", "read_bytes", "write_bytes"}));
    Relation http_events_relation;
    http_events_relation.AddColumn(types::TIME64NS, "time_");
    http_events_relation.AddColumn(types::UINT128, MetadataProperty::kUniquePIDColumn);
    http_events_relation.AddColumn(types::STRING, "remote_addr");
    http_events_relation.AddColumn(types::INT64, "remote_port");
    http_events_relation.AddColumn(types::INT64, "http_major_version");
    http_events_relation.AddColumn(types::INT64, "http_minor_version");
    http_events_relation.AddColumn(types::INT64, "http_content_type");
    http_events_relation.AddColumn(types::STRING, "http_req_headers");
    http_events_relation.AddColumn(types::STRING, "http_req_method");
    http_events_relation.AddColumn(types::STRING, "http_req_path");
    http_events_relation.AddColumn(types::STRING, "http_req_body");
    http_events_relation.AddColumn(types::STRING, "http_resp_headers");
    http_events_relation.AddColumn(types::INT64, "http_resp_status");
    http_events_relation.AddColumn(types::STRING, "http_resp_message");
    http_events_relation.AddColumn(types::STRING, "http_resp_body");
    http_events_relation.AddColumn(types::INT64, "http_resp_latency_ns");
    rel_map->emplace("http_events", http_events_relation);

    compiler_state_ = std::make_unique<CompilerState>(std::move(rel_map), info_.get(), time_now);

    compiler_ = Compiler();
  }
  std::unique_ptr<CompilerState> compiler_state_;
  std::unique_ptr<RegistryInfo> info_;
  int64_t time_now = 1552607213931245000;
  Compiler compiler_;
  Relation cgroups_relation_;
};

TEST_F(CompilerTest, test_general_compilation) {
  auto query = absl::StrJoin(
      {
          "import px",
          "queryDF = px.DataFrame(table='cpu', select=['cpu0', 'cpu1'])",
          "queryDF['quotient'] = queryDF['cpu1'] / queryDF['cpu0']",
          "aggDF = queryDF.groupby(['cpu0']).agg(",
          "quotient_mean=('quotient', px.mean),",
          "cpu1_mean=('cpu1', px.mean))",
          "px.display(aggDF, 'cpu2')",
      },
      "\n");
  auto plan_status = compiler_.CompileToIR(query, compiler_state_.get());
  ASSERT_OK(plan_status);

  auto ir = plan_status.ConsumeValueOrDie();
  auto operators = ir->FindNodesThatMatch(Operator());
  // figure out size.
  EXPECT_EQ(operators.size(), 4);

  auto srcs = ir->FindNodesThatMatch(MemorySource());
  EXPECT_EQ(srcs.size(), 1);
  auto mem_src = static_cast<MemorySourceIR*>(srcs[0]);
  EXPECT_EQ(mem_src->table_name(), "cpu");
  EXPECT_THAT(mem_src->column_names(), ElementsAre("cpu0", "cpu1"));

  auto child = mem_src->Children()[0];
  EXPECT_EQ(child->type(), IRNodeType::kMap);
  auto map = static_cast<MapIR*>(child);
  EXPECT_EQ(map->col_exprs().size(), 3);
  EXPECT_EQ(map->col_exprs()[0].name, "cpu0");
  EXPECT_EQ(map->col_exprs()[1].name, "cpu1");
  EXPECT_EQ(map->col_exprs()[2].name, "quotient");

  child = map->Children()[0];
  EXPECT_EQ(child->type(), IRNodeType::kBlockingAgg);

  auto agg = static_cast<BlockingAggIR*>(child);
  EXPECT_EQ(agg->groups().size(), 1);
  EXPECT_EQ(agg->groups()[0]->col_name(), "cpu0");
  EXPECT_EQ(agg->aggregate_expressions().size(), 2);
  EXPECT_EQ(agg->aggregate_expressions()[0].name, "quotient_mean");
  EXPECT_EQ(agg->aggregate_expressions()[1].name, "cpu1_mean");
  child = agg->Children()[0];
  EXPECT_EQ(child->type(), IRNodeType::kMemorySink);
  auto sink = static_cast<MemorySinkIR*>(child);
  EXPECT_EQ(sink->name(), "cpu2");
}

// Test for select order that is different than the schema.
constexpr char kSelectOrderLogicalPlan[] = R"(
nodes {
  nodes {
    op {
      op_type: MEMORY_SOURCE_OPERATOR
      mem_source_op {
        name: "cpu"
        column_idxs: 3
        column_idxs: 0
        column_idxs: 2
        column_names: "cpu2"
        column_names: "count"
        column_names: "cpu1"
        column_types: FLOAT64
        column_types: INT64
        column_types: FLOAT64
      }
    }
  }
  nodes {
    op {
      op_type: MEMORY_SINK_OPERATOR mem_sink_op {
        name: "cpu_out"
        column_types: FLOAT64
        column_types: INT64
        column_types: FLOAT64
        column_names: "cpu2"
        column_names: "count"
        column_names: "cpu1"
      }
    }
  }
}
)";

TEST_F(CompilerTest, select_order_test) {
  auto query = absl::StrJoin(
      {
          "import px",
          "queryDF = px.DataFrame(table='cpu', select=['cpu2', 'count', 'cpu1'])",
          "px.display(queryDF, 'cpu_out')",
      },
      "\n");

  auto plan = compiler_.Compile(query, compiler_state_.get());
  ASSERT_OK(plan);

  EXPECT_THAT(plan.ConsumeValueOrDie(), Partially(EqualsProto(kSelectOrderLogicalPlan)));
}

constexpr char kRangeNowPlan[] = R"(
nodes {
  nodes {
    op {
      op_type: MEMORY_SOURCE_OPERATOR
      mem_source_op {
        name: "sequences"
        column_idxs: 0
        column_idxs: 1
        column_names: "time_"
        column_names: "xmod10"
        column_types: TIME64NS
        column_types: FLOAT64
        start_time {
          value: $0
        }
        stop_time {
          value: $1
        }
      }
    }
  }
  nodes {
    op {
      op_type: MEMORY_SINK_OPERATOR
      mem_sink_op {
        name: "range_table"
        column_types: TIME64NS
        column_types: FLOAT64
        column_names: "time_"
        column_names: "xmod10"
      }
    }
  }
}

)";

TEST_F(CompilerTest, range_now_test) {
  auto query = absl::StrJoin(
      {
          "import px",
          "queryDF = px.DataFrame(table='sequences', select=['time_', 'xmod10'], start_time=0, "
          "end_time=px.now())",
          "px.display(queryDF,'range_table')",
      },
      "\n");

  auto plan = compiler_.Compile(query, compiler_state_.get());
  ASSERT_OK(plan);
  int64_t start_time = 0;
  int64_t stop_time = compiler_state_->time_now().val;
  auto expected_plan = absl::Substitute(kRangeNowPlan, start_time, stop_time);

  EXPECT_THAT(plan.ConsumeValueOrDie(), Partially(EqualsProto(expected_plan)));
}

constexpr char kRangeTimeUnitPlan[] = R"(
nodes {
  nodes {
    op {
      op_type: MEMORY_SOURCE_OPERATOR
      mem_source_op {
        name: "sequences"
        column_idxs: 0
        column_idxs: 1
        column_names: "time_"
        column_names: "xmod10"
        column_types: TIME64NS
        column_types: FLOAT64
        start_time {
          value: $0
        }
        stop_time {
          value: $1
        }
      }
    }
  }
  nodes {
    op {
      op_type: MEMORY_SINK_OPERATOR
      mem_sink_op {
        name: "$2"
        column_types: TIME64NS
        column_types: FLOAT64
        column_names: "time_"
        column_names: "xmod10"
        column_semantic_types: ST_NONE
        column_semantic_types: ST_NONE
      }
    }
  }
}

)";
class CompilerTimeFnTest
    : public CompilerTest,
      public ::testing::WithParamInterface<std::tuple<std::string, std::chrono::nanoseconds>> {
 protected:
  void SetUp() {
    CompilerTest::SetUp();
    std::tie(time_function, chrono_ns) = GetParam();
    query = absl::StrJoin({"import px",
                           "queryDF = px.DataFrame(table='sequences', select=['time_', "
                           "'xmod10'], start_time=px.now() - $1, end_time=px.now())",
                           "px.display(queryDF, '$0')"},
                          "\n");
    query = absl::Substitute(query, table_name_, time_function);
    VLOG(2) << query;
    int64_t now_time = compiler_state_->time_now().val;
    std::string expected_plan =
        absl::Substitute(kRangeTimeUnitPlan, now_time - chrono_ns.count(), now_time, table_name_);
    ASSERT_TRUE(google::protobuf::TextFormat::MergeFromString(expected_plan, &expected_plan_pb));
    VLOG(2) << expected_plan_pb.DebugString();
    compiler_ = Compiler();
  }
  std::string time_function;
  std::chrono::nanoseconds chrono_ns;
  std::string query;
  planpb::Plan expected_plan_pb;
  Compiler compiler_;

  std::string table_name_ = "range_table";
};

std::vector<std::tuple<std::string, std::chrono::nanoseconds>> compiler_time_data = {
    {"px.minutes(2)", std::chrono::minutes(2)},
    {"px.hours(2)", std::chrono::hours(2)},
    {"px.seconds(2)", std::chrono::seconds(2)},
    {"px.days(2)", std::chrono::hours(2 * 24)},
    {"px.microseconds(2)", std::chrono::microseconds(2)},
    {"px.milliseconds(2)", std::chrono::milliseconds(2)}};

TEST_P(CompilerTimeFnTest, range_now_keyword_test) {
  auto plan = compiler_.Compile(query, compiler_state_.get());
  ASSERT_OK(plan);
  VLOG(2) << plan.ValueOrDie().DebugString();

  EXPECT_TRUE(CompareLogicalPlans(expected_plan_pb, plan.ConsumeValueOrDie(), true /*ignore_ids*/));
}

INSTANTIATE_TEST_SUITE_P(CompilerTimeFnTestSuites, CompilerTimeFnTest,
                         ::testing::ValuesIn(compiler_time_data));

constexpr char kGroupByAllPlan[] = R"(
nodes {
  nodes {
    op {
      op_type: MEMORY_SOURCE_OPERATOR
      mem_source_op {
        name: "cpu"
        column_idxs: 1
        column_names: "cpu0"
        column_types: FLOAT64
      }
    }
  }
  nodes {
    op {
      op_type: AGGREGATE_OPERATOR
      agg_op {
        windowed: false
        values {
          name: "mean"
          args {
            column {
              index: 0
            }
          }
          args_data_types: FLOAT64
        }
        value_names: "mean"
      }
    }
  }
  nodes {
    op {
      op_type: MEMORY_SINK_OPERATOR
      mem_sink_op {
        name: "cpu_out"
        column_types: FLOAT64
        column_names: "mean"
      }
    }
  }
}
)";

TEST_F(CompilerTest, group_by_all) {
  auto query = absl::StrJoin(
      {
          "import px",
          "queryDF = px.DataFrame(table='cpu', select=['cpu1', 'cpu0'])",
          "aggDF = queryDF.agg(mean=('cpu0', px.mean))",
          "px.display(aggDF, 'cpu_out')",
      },
      "\n");

  auto plan_status = compiler_.Compile(query, compiler_state_.get());
  ASSERT_OK(plan_status);
  auto logical_plan = plan_status.ConsumeValueOrDie();
  VLOG(2) << logical_plan.DebugString();
  EXPECT_THAT(logical_plan, Partially(EqualsProto(kGroupByAllPlan)));
}

TEST_F(CompilerTest, multiple_group_by_agg_test) {
  std::string query =
      absl::StrJoin({"import px",
                     "queryDF = px.DataFrame(table='cpu', select=['cpu0', 'cpu1', 'cpu2'], "
                     "start_time=0, end_time=10)",
                     "aggDF = queryDF.groupby(['cpu0', 'cpu2']).agg(cpu_count=('cpu1', px.count),",
                     "cpu_mean=('cpu1', px.mean))", "px.display(aggDF, 'cpu_out')"},
                    "\n");

  auto plan = compiler_.Compile(query, compiler_state_.get());
  VLOG(2) << plan.ToString();
  ASSERT_OK(plan);
}

TEST_F(CompilerTest, multiple_group_by_map_then_agg) {
  std::string query =
      absl::StrJoin({"import px",
                     "queryDF = px.DataFrame(table='cpu', select=['cpu0', 'cpu1', 'cpu2'], "
                     "start_time=0, end_time=10)",
                     "queryDF.cpu_sum = queryDF.cpu1 + queryDF['cpu2']",
                     "aggDF = queryDF.groupby(['cpu0', 'cpu2']).agg(cpu_count=('cpu1', px.count),",
                     "cpu_mean=('cpu1', px.mean))", "px.display(aggDF, 'cpu_out')"},
                    "\n");

  auto plan = compiler_.Compile(query, compiler_state_.get());
  VLOG(2) << plan.ToString();
  ASSERT_OK(plan);
}

TEST_F(CompilerTest, rename_then_group_by_test) {
  auto query = absl::StrJoin(
      {"import px", "queryDF = px.DataFrame(table='sequences', select=['time_', 'xmod10', 'PIx'])",
       "queryDF['res'] = queryDF.PIx", "queryDF.c1 = queryDF['xmod10']",
       "map_out = queryDF[['res', 'c1']]",
       "agg_out = map_out.groupby(['res', 'c1']).agg(count=('c1', px.count))",
       "px.display(agg_out, 't15')"},
      "\n");

  auto plan = compiler_.Compile(query, compiler_state_.get());
  VLOG(2) << plan.ToString();
  ASSERT_OK(plan);
}

// Test to see whether comparisons work.
TEST_F(CompilerTest, comparison_test) {
  auto query = absl::StrJoin(
      {"import px", "queryDF = px.DataFrame(table='sequences', select=['time_', 'xmod10', 'PIx'])",
       "queryDF['res'] = queryDF['PIx']", "queryDF['c1'] = queryDF['xmod10']",
       "queryDF['gt'] = queryDF['xmod10'] > 10.0", "queryDF['lt'] = queryDF['xmod10'] < 10.0",
       "queryDF['gte'] = queryDF['PIx'] >= 1.0", "queryDF['lte'] = queryDF['PIx'] <= 1.0",
       "queryDF['eq'] = queryDF['PIx'] == 1.0",
       "map_out = queryDF[['res', 'c1', 'gt', 'lt', 'gte', 'lte', 'eq']]",
       "px.display(map_out, 't15')"},
      "\n");

  auto plan = compiler_.Compile(query, compiler_state_.get());
  VLOG(2) << plan.ToString();
  ASSERT_OK(plan);
}

constexpr char kAppendQuery[] = R"pxl(
import px
t1 = px.DataFrame(table='cpu', select=['cpu0'])
t2 = px.DataFrame(table='cpu', select=['cpu0'])
px.display(t1.append(t2))
)pxl";

TEST_F(CompilerTest, append_test) {
  auto plan = compiler_.Compile(kAppendQuery, compiler_state_.get());
  VLOG(2) << plan.ToString();
  ASSERT_OK(plan);
}

constexpr char kAppendSelfQuery[] = R"pxl(
import px
t1 = px.DataFrame(table='cpu', select=['cpu0'])
t2 = t1[t1.cpu0 > 0.0]
px.display(t1.append(t2))
)pxl";

TEST_F(CompilerTest, append_self_test) {
  auto plan = compiler_.Compile(kAppendSelfQuery, compiler_state_.get());
  VLOG(2) << plan.ToString();
  ASSERT_OK(plan);
}

constexpr char kFilterPlan[] = R"(
nodes {
  nodes {
    op {
      op_type: MEMORY_SOURCE_OPERATOR
      mem_source_op {
        name: "cpu"
        column_idxs: 1
        column_idxs: 2
        column_names: "cpu0"
        column_names: "cpu1"
        column_types: FLOAT64
        column_types: FLOAT64
      }
    }
  }
  nodes {
    op {
      op_type: FILTER_OPERATOR
      filter_op {
        expression {
          func {
            name: "$0"
            args {
              column {
              }
            }
            args {
              constant {
                data_type: FLOAT64
                float64_value: 0.5
              }
            }
            args_data_types: FLOAT64
            args_data_types: FLOAT64
          }
        }
        columns {
        }
        columns {
          index: 1
        }
      }
    }
  }
  nodes {
    op {
      op_type: MEMORY_SINK_OPERATOR
      mem_sink_op {
        name: "$1"
        column_types: FLOAT64
        column_types: FLOAT64
        column_names: "cpu0"
        column_names: "cpu1"
      }
    }
  }
}
)";

class FilterTest : public CompilerTest,
                   public ::testing::WithParamInterface<std::tuple<std::string, std::string>> {
 protected:
  void SetUp() {
    CompilerTest::SetUp();
    std::tie(compare_op_, compare_op_proto_) = GetParam();
    query =
        absl::StrJoin({"import px",
                       "queryDF = px.DataFrame(table='cpu', select=['cpu0', "
                       "'cpu1'])",
                       "queryDF = queryDF[queryDF['cpu0'] $0 0.5]", "px.display(queryDF, '$1')"},
                      "\n");
    query = absl::Substitute(query, compare_op_, table_name_);
    VLOG(2) << query;
    expected_plan = absl::Substitute(kFilterPlan, compare_op_proto_, table_name_);
  }
  std::string compare_op_;
  std::string compare_op_proto_;
  std::string query;
  std::string expected_plan;
  Compiler compiler_;

  std::string table_name_ = "range_table";
};

std::vector<std::tuple<std::string, std::string>> comparison_fns = {
    {">", "greaterThan"},       {"<", "lessThan"},       {"==", "equal"},
    {">=", "greaterThanEqual"}, {"<=", "lessThanEqual"}, {"!=", "notEqual"}};

TEST_P(FilterTest, basic) {
  auto plan = compiler_.Compile(query, compiler_state_.get());
  ASSERT_OK(plan);
  VLOG(2) << plan.ValueOrDie().DebugString();

  EXPECT_THAT(plan.ConsumeValueOrDie(), Partially(EqualsProto(expected_plan)));
}

INSTANTIATE_TEST_SUITE_P(FilterTestSuite, FilterTest, ::testing::ValuesIn(comparison_fns));

// TODO(nserrino/phlkuz) create expectations for filter errors.
TEST_F(CompilerTest, filter_errors) {
  std::string non_bool_filter =
      absl::StrJoin({"import px", "queryDF = px.DataFrame(table='cpu', select=['cpu0', 'cpu1'])",
                     "queryDF = queryDF[queryDF['cpu0'] + 0.5]", "px.display(queryDF, 'blah')"},
                    "\n");
  EXPECT_NOT_OK(compiler_.Compile(non_bool_filter, compiler_state_.get()));

  std::string int_val =
      absl::StrJoin({"import px", "queryDF = px.DataFrame(table='cpu', select=['cpu0', 'cpu1'])",
                     "d = queryDF[1]", "px.display(d, 'filtered')"},
                    "\n");
  EXPECT_NOT_OK(compiler_.Compile(int_val, compiler_state_.get()));
}

TEST_F(CompilerTest, reused_result) {
  std::string query = absl::StrJoin(
      {
          "import px",
          "queryDF = px.DataFrame(table='http_table', select=['time_', 'upid', 'http_resp_status', "
          "'http_resp_latency_ns'], start_time='-1m')",
          "x = queryDF[queryDF['http_resp_latency_ns'] < 1000000]",
          "px.display(queryDF, 'out');",
          "px.display(x);",
      },

      "\n");
  auto plan_status = compiler_.CompileToIR(query, compiler_state_.get());
  VLOG(2) << plan_status.ToString();
  ASSERT_OK(plan_status);
  auto plan = plan_status.ConsumeValueOrDie();
  std::vector<IRNode*> mem_srcs = plan->FindNodesOfType(IRNodeType::kMemorySource);
  ASSERT_EQ(mem_srcs.size(), 1);
  MemorySourceIR* mem_src = static_cast<MemorySourceIR*>(mem_srcs[0]);

  EXPECT_EQ(mem_src->Children().size(), 2);

  OperatorIR* src_child1 = mem_src->Children()[0];
  ASSERT_MATCH(src_child1, Filter());
  FilterIR* filter_child = static_cast<FilterIR*>(src_child1);
  EXPECT_MATCH(filter_child->filter_expr(), LessThan(ColumnNode(), Int(1000000)));
  EXPECT_EQ(filter_child->Children().size(), 1);

  OperatorIR* src_child2 = mem_src->Children()[1];
  ASSERT_MATCH(src_child2, MemorySink());
  MemorySinkIR* mem_sink_child = static_cast<MemorySinkIR*>(src_child2);
  EXPECT_EQ(mem_sink_child->name(), "out");
}

TEST_F(CompilerTest, multiple_result_sinks) {
  std::string query = absl::StrJoin(
      {
          "import px",
          "queryDF = px.DataFrame(table='http_table', select=['time_', 'upid', 'http_resp_status', "
          "'http_resp_latency_ns'], start_time='-1m')",
          "x = queryDF[queryDF['http_resp_latency_ns'] < "
          "1000000]",
          "px.display(x, 'filtered_result')",
          "px.display(queryDF, 'result');",
      },
      "\n");
  auto plan_status = compiler_.Compile(query, compiler_state_.get());
  ASSERT_OK(plan_status);
}

constexpr char kExpectedSelectDefaultArg[] = R"proto(
dag {
  nodes {
    id: 1
  }
}
nodes {
  id: 1
  dag {
    nodes {
      id: 1
      sorted_children: 0
    }
    nodes {
    }
  }
  nodes {
    id: 1
    op {
      op_type: MEMORY_SOURCE_OPERATOR
      mem_source_op {
        name: "cpu"
        column_idxs: 0
        column_idxs: 1
        column_idxs: 2
        column_idxs: 3
        column_idxs: 4
        column_names: "count"
        column_names: "cpu0"
        column_names: "cpu1"
        column_names: "cpu2"
        column_names: "upid"
        column_types: INT64
        column_types: FLOAT64
        column_types: FLOAT64
        column_types: FLOAT64
        column_types: UINT128
      }
    }
  }
  nodes {
    op {
      op_type: MEMORY_SINK_OPERATOR
      mem_sink_op {
        name: "out"
        column_types: INT64
        column_types: FLOAT64
        column_types: FLOAT64
        column_types: FLOAT64
        column_types: UINT128
        column_names: "count"
        column_names: "cpu0"
        column_names: "cpu1"
        column_names: "cpu2"
        column_names: "upid"
        column_semantic_types: ST_NONE
        column_semantic_types: ST_NONE
        column_semantic_types: ST_NONE
        column_semantic_types: ST_NONE
        column_semantic_types: ST_NONE
      }
    }
  }
}
)proto";
TEST_F(CompilerTest, from_select_default_arg) {
  std::string no_select_arg = "import px\ndf = px.DataFrame(table='cpu')\npx.display(df, 'out')";
  auto plan_status = compiler_.Compile(no_select_arg, compiler_state_.get());
  ASSERT_OK(plan_status);
  auto plan = plan_status.ValueOrDie();
  VLOG(2) << plan.DebugString();

  // Check the select columns match the expected values.
  planpb::Plan expected_plan_pb;
  ASSERT_TRUE(
      google::protobuf::TextFormat::MergeFromString(kExpectedSelectDefaultArg, &expected_plan_pb));
  EXPECT_TRUE(CompareLogicalPlans(expected_plan_pb, plan, true /*ignore_ids*/));
}

constexpr char kExpectedFilterMetadataPlan[] = R"proto(
dag {
  nodes {
    id: 1
  }
}
nodes {
  id: 1
  dag {
    nodes {
      id: 6
      sorted_children: 11
    }
    nodes {
      id: 11
      sorted_children: 13
      sorted_parents: 6
    }
    nodes {
      id: 13
      sorted_parents: 11
    }
  }
  nodes {
    id: 6
    op {
      op_type: MEMORY_SOURCE_OPERATOR
      mem_source_op {
        name: "cpu"
        column_idxs: 0
        column_idxs: 1
        column_idxs: 2
        column_idxs: 3
        column_idxs: 4
        column_names: "count"
        column_names: "cpu0"
        column_names: "cpu1"
        column_names: "cpu2"
        column_names: "upid"
        column_types: INT64
        column_types: FLOAT64
        column_types: FLOAT64
        column_types: FLOAT64
        column_types: UINT128
      }
    }
  }
  nodes {
    id: 11
    op {
      op_type: FILTER_OPERATOR
      filter_op {
        expression {
          func {
            name: "equal"
            args {
              func {
                name: "upid_to_service_name"
                args {
                  column {
                    node: 6
                    index: 4
                  }
                }
                id: 1
                args_data_types: UINT128
              }
            }
            args {
              constant {
                data_type: STRING
                string_value: "pl/orders"
              }
            }
            args_data_types: STRING
            args_data_types: STRING
          }
        }
        columns {
          node: 6
        }
        columns {
          node: 6
          index: 1
        }
        columns {
          node: 6
          index: 2
        }
        columns {
          node: 6
          index: 3
        }
        columns {
          node: 6
          index: 4
        }
      }
    }
  }
  nodes {
    id: 13
    op {
      op_type: MEMORY_SINK_OPERATOR
      mem_sink_op {
        name: "out"
        column_types: INT64
        column_types: FLOAT64
        column_types: FLOAT64
        column_types: FLOAT64
        column_types: UINT128
        column_names: "count"
        column_names: "cpu0"
        column_names: "cpu1"
        column_names: "cpu2"
        column_names: "upid"
      }
    }
  }
}
)proto";

constexpr char kExpectedMapMetadataPlan[] = R"proto(
dag {
  nodes {
    id: 1
  }
}
nodes {
  id: 1
  dag {
    nodes {
      id: 6
      sorted_children: 11
    }
    nodes {
      id: 11
      sorted_children: 14
      sorted_parents: 6
    }
    nodes {
      id: 14
      sorted_children: 16
      sorted_parents: 11
    }
    nodes {
      id: 16
      sorted_parents: 14
    }
  }
  nodes {
    id: 6
    op {
      op_type: MEMORY_SOURCE_OPERATOR
      mem_source_op {
        name: "cpu"
        column_idxs: 4
        column_names: "upid"
        column_types: UINT128
      }
    }
  }
  nodes {
    id: 11
    op {
      op_type: MAP_OPERATOR
      map_op {
        expressions {
          func {
            name: "upid_to_service_name"
            args {
              column {
                node: 6
              }
            }
            args_data_types: UINT128
          }
        }
        column_names: "service"
      }
    }
  }
  nodes {
    id: 14
    op {
      op_type: MAP_OPERATOR
      map_op {
        expressions {
          column {
            node: 11
          }
        }
        column_names: "service"
      }
    }
  }
  nodes {
    id: 16
    op {
      op_type: MEMORY_SINK_OPERATOR
      mem_sink_op {
        name: "out"
        column_types: STRING
        column_names: "service"
      }
    }
  }
}
)proto";

constexpr char kExpectedAgg1MetadataPlan[] = R"proto(
dag {
  nodes {
    id: 1
  }
}
nodes {
  id: 1
  dag {
    nodes {
      id: 6
      sorted_children: 11
    }
    nodes {
      id: 11
      sorted_children: 18
      sorted_parents: 6
    }
    nodes {
      id: 18
      sorted_children: 20
      sorted_parents: 11
    }
    nodes {
      id: 20
      sorted_parents: 18
    }
  }
  nodes {
    id: 6
    op {
      op_type: MEMORY_SOURCE_OPERATOR
      mem_source_op {
        name: "cpu"
        column_idxs: 1
        column_idxs: 4
        column_names: "cpu0"
        column_names: "upid"
        column_types: FLOAT64
        column_types: UINT128
      }
    }
  }
  nodes {
    id: 11
    op {
      op_type: MAP_OPERATOR
      map_op {
        expressions {
          column {
            node: 6
          }
        }
        expressions {
          func {
            name: "upid_to_service_name"
            args {
              column {
                node: 6
                index: 1
              }
            }
            args_data_types: UINT128
          }
        }
        column_names: "cpu0"
        column_names: "service"
      }
    }
  }
  nodes {
    id: 18
    op {
      op_type: AGGREGATE_OPERATOR
      agg_op {
        values {
          name: "mean"
          args {
            column {
              node: 11
            }
          }
          args_data_types: FLOAT64
        }
        groups {
          node: 11
          index: 1
        }
        group_names: "service"
        value_names: "mean_cpu"
      }
    }
  }
  nodes {
    id: 20
    op {
      op_type: MEMORY_SINK_OPERATOR
      mem_sink_op {
        name: "out"
        column_types: STRING
        column_types: FLOAT64
        column_names: "service"
        column_names: "mean_cpu"
      }
    }
  }
}
)proto";

constexpr char kExpectedAgg2MetadataPlan[] = R"proto(
dag {
  nodes {
    id: 1
  }
}
nodes {
  id: 1
  dag {
    nodes {
      id: 6
      sorted_children: 11
    }
    nodes {
      id: 11
      sorted_children: 20
      sorted_parents: 6
    }
    nodes {
      id: 20
      sorted_children: 22
      sorted_parents: 11
    }
    nodes {
      id: 22
      sorted_parents: 20
    }
  }
  nodes {
    id: 6
    op {
      op_type: MEMORY_SOURCE_OPERATOR
      mem_source_op {
        name: "cpu"
        column_idxs: 1
        column_idxs: 4
        column_names: "cpu0"
        column_names: "upid"
        column_types: FLOAT64
        column_types: UINT128
      }
    }
  }
  nodes {
    id: 11
    op {
      op_type: MAP_OPERATOR
      map_op {
        expressions {
          column {
            node: 6
          }
        }
        expressions {
          func {
            name: "upid_to_service_name"
            args {
              column {
                node: 6
                index: 1
              }
            }
            args_data_types: UINT128
          }
        }
        column_names: "cpu0"
        column_names: "service"
      }
    }
  }
  nodes {
    id: 20
    op {
      op_type: AGGREGATE_OPERATOR
      agg_op {
        values {
          name: "mean"
          args {
            column {
              node: 11
            }
          }
          args_data_types: FLOAT64
        }
        groups {
          node: 11
        }
        groups {
          node: 11
          index: 1
        }
        group_names: "cpu0"
        group_names: "service"
        value_names: "mean_cpu"
      }
    }
  }
  nodes {
    id: 22
    op {
      op_type: MEMORY_SINK_OPERATOR
      mem_sink_op {
        name: "out"
        column_types: FLOAT64
        column_types: STRING
        column_types: FLOAT64
        column_names: "cpu0"
        column_names: "service"
        column_names: "mean_cpu"
      }
    }
  }
}
)proto";

constexpr char kExpectedAggFilter1MetadataPlan[] = R"proto(
dag {
  nodes {
    id: 1
  }
}
nodes {
  id: 1
  dag {
    nodes {
      id: 6
      sorted_children: 25
    }
    nodes {
      id: 25
      sorted_children: 11
      sorted_parents: 6
    }
    nodes {
      id: 11
      sorted_children: 20
      sorted_parents: 25
    }
    nodes {
      id: 20
      sorted_children: 27
      sorted_parents: 11
    }
    nodes {
      id: 27
      sorted_parents: 20
    }
  }
  nodes {
    id: 6
    op {
      op_type: MEMORY_SOURCE_OPERATOR
      mem_source_op {
        name: "cpu"
        column_idxs: 1
        column_idxs: 4
        column_names: "cpu0"
        column_names: "upid"
        column_types: FLOAT64
        column_types: UINT128
      }
    }
  }
  nodes {
    id: 25
    op {
      op_type: FILTER_OPERATOR
      filter_op {
        expression {
          func {
            name: "equal"
            args {
              func {
                name: "upid_to_service_name"
                args {
                  column {
                    node: 6
                    index: 1
                  }
                }
                id: 1
                args_data_types: UINT128
              }
            }
            args {
              constant {
                data_type: STRING
                string_value: "pl/service-name"
              }
            }
            args_data_types: STRING
            args_data_types: STRING
          }
        }
        columns {
          node: 6
        }
        columns {
          node: 6
          index: 1
        }
      }
    }
  }
  nodes {
    id: 11
    op {
      op_type: MAP_OPERATOR
      map_op {
        expressions {
          column {
            node: 25
          }
        }
        expressions {
          column {
            node: 25
            index: 1
          }
        }
        expressions {
          func {
            name: "upid_to_service_name"
            args {
              column {
                node: 25
                index: 1
              }
            }
            id: 1
            args_data_types: UINT128
          }
        }
        column_names: "cpu0"
        column_names: "upid"
        column_names: "service"
      }
    }
  }
  nodes {
    id: 20
    op {
      op_type: AGGREGATE_OPERATOR
      agg_op {
        values {
          name: "mean"
          args {
            column {
              node: 11
            }
          }
          args_data_types: FLOAT64
        }
        groups {
          node: 11
          index: 1
        }
        groups {
          node: 11
          index: 2
        }
        group_names: "upid"
        group_names: "service"
        value_names: "mean_cpu"
      }
    }
  }
  nodes {
    id: 27
    op {
      op_type: MEMORY_SINK_OPERATOR
      mem_sink_op {
        name: "out"
        column_types: UINT128
        column_types: STRING
        column_types: FLOAT64
        column_names: "upid"
        column_names: "service"
        column_names: "mean_cpu"
      }
    }
  }
}
)proto";

constexpr char kExpectedAggFilter2MetadataPlan[] = R"proto(
nodes {
  id: 1
  dag {
    nodes {
      id: 2
      sorted_children: 28
    }
    nodes {
      id: 28
      sorted_children: 10
      sorted_parents: 2
    }
    nodes {
      id: 10
      sorted_children: 15
      sorted_parents: 28
    }
    nodes {
      id: 15
      sorted_children: 17
      sorted_parents: 10
    }
    nodes {
      id: 17
      sorted_parents: 15
    }
  }
  nodes {
    id: 2
    op {
      op_type: MEMORY_SOURCE_OPERATOR
      mem_source_op {
        name: "cpu"
        column_idxs: 0
        column_idxs: 1
        column_idxs: 2
        column_idxs: 3
        column_idxs: 4
        column_names: "count"
        column_names: "cpu0"
        column_names: "cpu1"
        column_names: "cpu2"
        column_names: "upid"
        column_types: INT64
        column_types: FLOAT64
        column_types: FLOAT64
        column_types: FLOAT64
        column_types: UINT128
      }
    }
  }
  nodes {
    id: 28
    op {
      op_type: MAP_OPERATOR
      map_op {
        expressions {
          column {
          }
        }
        expressions {
          column {
            index: 1
          }
        }
        expressions {
          column {
            index: 2
          }
        }
        expressions {
          column {
            index: 3
          }
        }
        expressions {
          column {
            index: 4
          }
        }
        expressions {
          func {
            name: "upid_to_service_name"
            args {
              column {
                index: 4
              }
            }
            id: 1
            args_data_types: UINT128
          }
        }
        column_names: "count"
        column_names: "cpu0"
        column_names: "cpu1"
        column_names: "cpu2"
        column_names: "upid"
        column_names: "_attr_service_name"
      }
    }
  }
  nodes {
    id: 10
    op {
      op_type: AGGREGATE_OPERATOR
      agg_op {
        values {
          name: "mean"
          args {
            column {
              index: 1
            }
          }
          args_data_types: FLOAT64
        }
        groups {
          index: 1
        }
        groups {
          index: 5
        }
        group_names: "cpu0"
        group_names: "_attr_service_name"
        value_names: "mean_cpu"
      }
    }
  }
  nodes {
    id: 15
    op {
      op_type: FILTER_OPERATOR
      filter_op {
        expression {
          func {
            name: "equal"
            args {
              column {
                index: 1
              }
            }
            args {
              constant {
                data_type: STRING
                string_value: "pl/orders"
              }
            }
            args_data_types: STRING
            args_data_types: STRING
          }
        }
        columns {
        }
        columns {
          index: 1
        }
        columns {
          index: 2
        }
      }
    }
  }
  nodes {
    id: 17
    op {
      op_type: MEMORY_SINK_OPERATOR
      mem_sink_op {
        name: "out"
        column_types: FLOAT64
        column_types: STRING
        column_types: FLOAT64
        column_names: "cpu0"
        column_names: "_attr_service_name"
        column_names: "mean_cpu"
      }
    }
  }
}
)proto";

constexpr char kExpectedAliasingMetadataPlan[] = R"proto(
nodes {
  id: 1
  dag {
    nodes {
      id: 2
      sorted_children: 28
    }
    nodes {
      id: 28
      sorted_children: 10
      sorted_parents: 2
    }
    nodes {
      id: 10
      sorted_children: 34
      sorted_parents: 28
    }
    nodes {
      id: 34
      sorted_children: 15
      sorted_parents: 10
    }
    nodes {
      id: 15
      sorted_children: 17
      sorted_parents: 34
    }
    nodes {
      id: 17
      sorted_parents: 15
    }
  }
  nodes {
    id: 2
    op {
      op_type: MEMORY_SOURCE_OPERATOR
      mem_source_op {
        name: "cpu"
        column_idxs: 0
        column_idxs: 1
        column_idxs: 2
        column_idxs: 3
        column_idxs: 4
        column_names: "count"
        column_names: "cpu0"
        column_names: "cpu1"
        column_names: "cpu2"
        column_names: "upid"
        column_types: INT64
        column_types: FLOAT64
        column_types: FLOAT64
        column_types: FLOAT64
        column_types: UINT128
      }
    }
  }
  nodes {
    id: 28
    op {
      op_type: MAP_OPERATOR
      map_op {
        expressions {
          column {
          }
        }
        expressions {
          column {
            index: 1
          }
        }
        expressions {
          column {
            index: 2
          }
        }
        expressions {
          column {
            index: 3
          }
        }
        expressions {
          column {
            index: 4
          }
        }
        expressions {
          func {
            name: "upid_to_service_id"
            args {
              column {
                index: 4
              }
            }
            id: 1
            args_data_types: UINT128
          }
        }
        column_names: "count"
        column_names: "cpu0"
        column_names: "cpu1"
        column_names: "cpu2"
        column_names: "upid"
        column_names: "_attr_service_id"
      }
    }
  }
  nodes {
    id: 10
    op {
      op_type: AGGREGATE_OPERATOR
      agg_op {
        values {
          name: "mean"
          args {
            column {
              index: 1
            }
          }
          args_data_types: FLOAT64
        }
        groups {
          index: 1
        }
        groups {
          index: 5
        }
        group_names: "cpu0"
        group_names: "_attr_service_id"
        value_names: "mean_cpu"
      }
    }
  }
  nodes {
    id: 34
    op {
      op_type: MAP_OPERATOR
      map_op {
        expressions {
          column {
          }
        }
        expressions {
          column {
            index: 1
          }
        }
        expressions {
          column {
            index: 2
          }
        }
        expressions {
          func {
            name: "service_id_to_service_name"
            args {
              column {
                index: 1
              }
            }
            id: 2
            args_data_types: STRING
          }
        }
        column_names: "cpu0"
        column_names: "_attr_service_id"
        column_names: "mean_cpu"
        column_names: "_attr_service_name"
      }
    }
  }
  nodes {
    id: 15
    op {
      op_type: FILTER_OPERATOR
      filter_op {
        expression {
          func {
            name: "equal"
            args {
              column {
                index: 3
              }
            }
            args {
              constant {
                data_type: STRING
                string_value: "pl/orders"
              }
            }
            args_data_types: STRING
            args_data_types: STRING
          }
        }
        columns {
        }
        columns {
          index: 1
        }
        columns {
          index: 2
        }
        columns {
          index: 3
        }
      }
    }
  }
  nodes {
    id: 17
    op {
      op_type: MEMORY_SINK_OPERATOR
      mem_sink_op {
        name: "out"
        column_types: FLOAT64
        column_types: STRING
        column_types: FLOAT64
        column_types: STRING
        column_names: "cpu0"
        column_names: "_attr_service_id"
        column_names: "mean_cpu"
        column_names: "_attr_service_name"
      }
    }
  }
}
)proto";

class MetadataSingleOps
    : public CompilerTest,
      public ::testing::WithParamInterface<std::tuple<std::string, std::string>> {};

// Having this indirection makes reading failed tests much easier.
absl::flat_hash_map<std::string, std::string> metadata_name_to_plan_map{
    {"filter_metadata_plan", kExpectedFilterMetadataPlan},
    {"map_metadata_plan", kExpectedMapMetadataPlan},
    {"agg_metadata_plan1", kExpectedAgg1MetadataPlan},
    {"agg_metadata_plan2", kExpectedAgg2MetadataPlan},
    {"agg_filter_metadata_plan1", kExpectedAggFilter1MetadataPlan},
    {"agg_filter_metadata_plan2", kExpectedAggFilter2MetadataPlan},
    {"aliasing_metadata_plan", kExpectedAliasingMetadataPlan},
};

TEST_P(MetadataSingleOps, valid_filter_metadata_proto) {
  std::string op_call, expected_pb_name;
  std::tie(op_call, expected_pb_name) = GetParam();
  auto expected_pb_iter = metadata_name_to_plan_map.find(expected_pb_name);
  ASSERT_TRUE(expected_pb_iter != metadata_name_to_plan_map.end()) << expected_pb_name;
  std::string expected_pb = expected_pb_iter->second;
  std::string valid_query = absl::StrJoin(
      {"import px", "df = px.DataFrame(table='cpu') ", "$0", "px.display(df, 'out')"}, "\n");
  valid_query = absl::Substitute(valid_query, op_call);

  auto plan_status = compiler_.Compile(valid_query, compiler_state_.get());
  ASSERT_OK(plan_status) << valid_query;

  auto plan = plan_status.ConsumeValueOrDie();
  // Check the select columns match the expected values.
  EXPECT_THAT(plan, Partially(EqualsProto(expected_pb)))
      << absl::Substitute("Actual proto for $0: $1", expected_pb_name, plan.DebugString());
}

// Indirectly maps to metadata_name_to_plan_map
std::vector<std::tuple<std::string, std::string>> metadata_operators{
    {"df = df[df.ctx['service'] == 'pl/orders']", "filter_metadata_plan"},
    {"df['service'] = df.ctx['service']\ndf = df[['service']]", "map_metadata_plan"},
    {"df['service'] =  df.ctx['service']\n"
     "df = df.groupby('service').agg(mean_cpu = ('cpu0', px.mean))",
     "agg_metadata_plan1"},
    {"df['service'] =  df.ctx['service']\n"
     "df = df.groupby(['cpu0', 'service']).agg(mean_cpu = ('cpu0', px.mean))",
     "agg_metadata_plan2"},
    {"df['service'] =  df.ctx['service']\n"
     "aggDF = df.groupby(['upid', 'service']).agg(mean_cpu = ('cpu0', px.mean))\n"
     "df =aggDF[aggDF.ctx['service'] == 'pl/service-name']",
     "agg_filter_metadata_plan1"}};

INSTANTIATE_TEST_SUITE_P(MetadataAttributesSuite, MetadataSingleOps,
                         ::testing::ValuesIn(metadata_operators));

TEST_F(CompilerTest, cgroups_pod_id) {
  std::string query =
      absl::StrJoin({"import px", "queryDF = px.DataFrame(table='cgroups')",
                     "range_out = queryDF[queryDF.ctx['pod_name'] == 'pl/pl-nats-1']",
                     "px.display(range_out, 'out')"},
                    "\n");
  auto plan_status = compiler_.Compile(query, compiler_state_.get());
  ASSERT_OK(plan_status);
}

constexpr char kJoinInnerQueryPlan[] = R"proto(
  dag {
  nodes {
    id: 1
  }
}
nodes {
  id: 1
  dag {
    nodes {
      id: 9
      sorted_children: 24
    }
    nodes {
      id: 16
      sorted_children: 24
    }
    nodes {
      id: 24
      sorted_children: 35
      sorted_parents: 9
      sorted_parents: 16
    }
    nodes {
      id: 35
      sorted_children: 37
      sorted_parents: 24
    }
    nodes {
      id: 37
      sorted_parents: 35
    }
  }
  nodes {
    id: 9
    op {
      op_type: MEMORY_SOURCE_OPERATOR
      mem_source_op {
        name: "cpu"
        column_idxs: 1
        column_idxs: 4
        column_idxs: 2
        column_names: "cpu0"
        column_names: "upid"
        column_names: "cpu1"
        column_types: FLOAT64
        column_types: UINT128
        column_types: FLOAT64
      }
    }
  }
  nodes {
    id: 16
    op {
      op_type: MEMORY_SOURCE_OPERATOR
      mem_source_op {
        name: "http_table"
        column_idxs: 2
        column_idxs: 1
        column_idxs: 3
        column_names: "http_resp_status"
        column_names: "upid"
        column_names: "http_resp_latency_ns"
        column_types: INT64
        column_types: UINT128
        column_types: INT64
      }
    }
  }
  nodes {
    id: 24
    op {
      op_type: JOIN_OPERATOR
      join_op {
        equality_conditions {
          left_column_index: 1
          right_column_index: 1
        }
        output_columns {
        }
        output_columns {
          column_index: 1
        }
        output_columns {
          column_index: 2
        }
        output_columns {
          parent_index: 1
        }
        output_columns {
          parent_index: 1
          column_index: 2
        }
        column_names: "cpu0"
        column_names: "upid"
        column_names: "cpu1"
        column_names: "http_resp_status"
        column_names: "http_resp_latency_ns"
      }
    }
  }
  nodes {
    id: 35
    op {
      op_type: MAP_OPERATOR
      map_op {
        expressions {
          column {
            node: 24
            index: 1
          }
        }
        expressions {
          column {
            node: 24
            index: 3
          }
        }
        expressions {
          column {
            node: 24
            index: 4
          }
        }
        expressions {
          column {
            node: 24
          }
        }
        expressions {
          column {
            node: 24
            index: 2
          }
        }
        column_names: "upid"
        column_names: "http_resp_status"
        column_names: "http_resp_latency_ns"
        column_names: "cpu0"
        column_names: "cpu1"
      }
    }
  }
  nodes {
    id: 37
    op {
      op_type: MEMORY_SINK_OPERATOR
      mem_sink_op {
        name: "joined"
        column_types: UINT128
        column_types: INT64
        column_types: INT64
        column_types: FLOAT64
        column_types: FLOAT64
        column_names: "upid"
        column_names: "http_resp_status"
        column_names: "http_resp_latency_ns"
        column_names: "cpu0"
        column_names: "cpu1"
        column_semantic_types: ST_NONE
        column_semantic_types: ST_NONE
        column_semantic_types: ST_NONE
        column_semantic_types: ST_NONE
        column_semantic_types: ST_NONE
      }
    }
  }
}
)proto";

constexpr char kJoinQueryTypeTpl[] = R"query(
import px
src1 = px.DataFrame(table='cpu', select=['cpu0', 'upid', 'cpu1'])
src2 = px.DataFrame(table='http_table', select=['http_resp_status', 'upid',  'http_resp_latency_ns'])
join = src1.merge(src2, how='$0', left_on=['upid'], right_on=['upid'], suffixes=['', '_x'])
output = join[["upid", "http_resp_status", "http_resp_latency_ns", "cpu0", "cpu1"]]
px.display(output, 'joined')
)query";

TEST_F(CompilerTest, inner_join) {
  auto plan_status =
      compiler_.Compile(absl::Substitute(kJoinQueryTypeTpl, "inner"), compiler_state_.get());
  ASSERT_OK(plan_status);
  auto plan = plan_status.ConsumeValueOrDie();
  EXPECT_THAT(plan, EqualsProto(kJoinInnerQueryPlan)) << plan.DebugString();
}

constexpr char kJoinRightQueryPlan[] = R"proto(
  dag {
  nodes {
    id: 1
  }
}
nodes {
  id: 1
  dag {
    nodes {
      id: 9
      sorted_children: 24
    }
    nodes {
      id: 16
      sorted_children: 24
    }
    nodes {
      id: 24
      sorted_children: 35
      sorted_parents: 16
      sorted_parents: 9
    }
    nodes {
      id: 35
      sorted_children: 37
      sorted_parents: 24
    }
    nodes {
      id: 37
      sorted_parents: 35
    }
  }
  nodes {
    id: 9
    op {
      op_type: MEMORY_SOURCE_OPERATOR
      mem_source_op {
        name: "cpu"
        column_idxs: 1
        column_idxs: 4
        column_idxs: 2
        column_names: "cpu0"
        column_names: "upid"
        column_names: "cpu1"
        column_types: FLOAT64
        column_types: UINT128
        column_types: FLOAT64
      }
    }
  }
  nodes {
    id: 16
    op {
      op_type: MEMORY_SOURCE_OPERATOR
      mem_source_op {
        name: "http_table"
        column_idxs: 2
        column_idxs: 1
        column_idxs: 3
        column_names: "http_resp_status"
        column_names: "upid"
        column_names: "http_resp_latency_ns"
        column_types: INT64
        column_types: UINT128
        column_types: INT64
      }
    }
  }
  nodes {
    id: 24
    op {
      op_type: JOIN_OPERATOR
      join_op {
        type: LEFT_OUTER
        equality_conditions {
          left_column_index: 1
          right_column_index: 1
        }
        output_columns {
          parent_index: 1
        }
        output_columns {
          parent_index: 1
          column_index: 1
        }
        output_columns {
          parent_index: 1
          column_index: 2
        }
        output_columns {
        }
        output_columns {
          column_index: 2
        }
        column_names: "cpu0"
        column_names: "upid"
        column_names: "cpu1"
        column_names: "http_resp_status"
        column_names: "http_resp_latency_ns"
      }
    }
  }
  nodes {
    id: 35
    op {
      op_type: MAP_OPERATOR
      map_op {
        expressions {
          column {
            node: 24
            index: 1
          }
        }
        expressions {
          column {
            node: 24
            index: 3
          }
        }
        expressions {
          column {
            node: 24
            index: 4
          }
        }
        expressions {
          column {
            node: 24
          }
        }
        expressions {
          column {
            node: 24
            index: 2
          }
        }
        column_names: "upid"
        column_names: "http_resp_status"
        column_names: "http_resp_latency_ns"
        column_names: "cpu0"
        column_names: "cpu1"
      }
    }
  }
  nodes {
    id: 37
    op {
      op_type: MEMORY_SINK_OPERATOR
      mem_sink_op {
        name: "joined"
        column_types: UINT128
        column_types: INT64
        column_types: INT64
        column_types: FLOAT64
        column_types: FLOAT64
        column_names: "upid"
        column_names: "http_resp_status"
        column_names: "http_resp_latency_ns"
        column_names: "cpu0"
        column_names: "cpu1"
        column_semantic_types: ST_NONE
        column_semantic_types: ST_NONE
        column_semantic_types: ST_NONE
        column_semantic_types: ST_NONE
        column_semantic_types: ST_NONE
      }
    }
  }
}
)proto";

// TODO(philkuz/nserrino): Fix test broken with clang-9/gcc-9.
TEST_F(CompilerTest, right_join) {
  auto plan_status =
      compiler_.Compile(absl::Substitute(kJoinQueryTypeTpl, "right"), compiler_state_.get());
  ASSERT_OK(plan_status);
  auto plan = plan_status.ConsumeValueOrDie();
  EXPECT_THAT(plan, EqualsProto(kJoinRightQueryPlan)) << plan.DebugString();
}

constexpr char kSelfJoinQueryPlan[] = R"proto(
  dag {
  nodes {
    id: 1
  }
}
nodes {
  id: 1
  dag {
    nodes {
      id: 9
      sorted_children: 18
      sorted_children: 17
    }
    nodes {
      id: 18
      sorted_children: 17
      sorted_parents: 9
    }
    nodes {
      id: 17
      sorted_children: 20
      sorted_parents: 9
      sorted_parents: 18
    }
    nodes {
      id: 20
      sorted_parents: 17
    }
  }
  nodes {
    id: 9
    op {
      op_type: MEMORY_SOURCE_OPERATOR
      mem_source_op {
        name: "cpu"
        column_idxs: 1
        column_idxs: 4
        column_idxs: 2
        column_names: "cpu0"
        column_names: "upid"
        column_names: "cpu1"
        column_types: FLOAT64
        column_types: UINT128
        column_types: FLOAT64
      }
    }
  }
  nodes {
    id: 18
    op {
      op_type: MAP_OPERATOR
      map_op {
        expressions {
          column {
            node: 9
          }
        }
        expressions {
          column {
            node: 9
            index: 1
          }
        }
        expressions {
          column {
            node: 9
            index: 2
          }
        }
        column_names: "cpu0"
        column_names: "upid"
        column_names: "cpu1"
      }
    }
  }
  nodes {
    id: 17
    op {
      op_type: JOIN_OPERATOR
      join_op {
        equality_conditions {
          left_column_index: 1
          right_column_index: 1
        }
        output_columns {
        }
        output_columns {
          column_index: 1
        }
        output_columns {
          column_index: 2
        }
        output_columns {
          parent_index: 1
        }
        output_columns {
          parent_index: 1
          column_index: 1
        }
        output_columns {
          parent_index: 1
          column_index: 2
        }
        column_names: "cpu0"
        column_names: "upid"
        column_names: "cpu1"
        column_names: "cpu0_x"
        column_names: "upid_x"
        column_names: "cpu1_x"
      }
    }
  }
  nodes {
    id: 20
    op {
      op_type: MEMORY_SINK_OPERATOR
      mem_sink_op {
        name: "joined"
        column_types: FLOAT64
        column_types: UINT128
        column_types: FLOAT64
        column_types: FLOAT64
        column_types: UINT128
        column_types: FLOAT64
        column_names: "cpu0"
        column_names: "upid"
        column_names: "cpu1"
        column_names: "cpu0_x"
        column_names: "upid_x"
        column_names: "cpu1_x"
        column_semantic_types: ST_NONE
        column_semantic_types: ST_NONE
        column_semantic_types: ST_NONE
        column_semantic_types: ST_NONE
        column_semantic_types: ST_NONE
        column_semantic_types: ST_NONE
      }
    }
  }
}

)proto";

constexpr char kSelfJoinQuery[] = R"query(
import px
src1 = px.DataFrame(table='cpu', select=['cpu0', 'upid', 'cpu1'])
join = src1.merge(src1, how='inner', left_on=['upid'], right_on=['upid'], suffixes=['', '_x'])
px.display(join, 'joined')
)query";

TEST_F(CompilerTest, self_join) {
  auto plan_status = compiler_.Compile(kSelfJoinQuery, compiler_state_.get());
  ASSERT_OK(plan_status);
  auto plan = plan_status.ConsumeValueOrDie();
  EXPECT_THAT(plan, EqualsProto(kSelfJoinQueryPlan)) << "ACTUAL PLAN: " << plan.DebugString();
}

// Test to make sure syntax errors are properly parsed.
TEST_F(CompilerTest, syntax_error_test) {
  auto syntax_error_query = "import px\npx.DataFrame(";
  auto plan_status = compiler_.Compile(syntax_error_query, compiler_state_.get());
  ASSERT_NOT_OK(plan_status);
  EXPECT_THAT(plan_status.status(), HasCompilerError("SyntaxError: Expected `\\)`"));
}

TEST_F(CompilerTest, indentation_error_test) {
  auto indent_error_query = absl::StrJoin(
      {"import px", "t = px.DataFrame(table='blah')", "    px.display(t, 'blah')"}, "\n");
  auto plan_status = compiler_.Compile(indent_error_query, compiler_state_.get());
  ASSERT_NOT_OK(plan_status);
  EXPECT_THAT(plan_status.status(), HasCompilerError("SyntaxError: invalid syntax"));
}

TEST_F(CompilerTest, missing_result) {
  // Missing the result call at the end of the query.
  auto missing_result_call =
      "import px\nqueryDF = px.DataFrame(table='cpu', select=['cpu0', 'cpu1'])";
  auto missing_result_status = compiler_.Compile(missing_result_call, compiler_state_.get());
  ASSERT_NOT_OK(missing_result_status);

  EXPECT_THAT(missing_result_status.status().msg(),
              ContainsRegex("query does not output a result, please add a px.display.* statement"));
}

constexpr char kBadDropQuery[] = R"pxl(
import px
t1 = px.DataFrame(table='http_events', start_time='-300s')
t1['service'] = t1.ctx['service']
t1['http_resp_latency_ms'] = t1['http_resp_latency_ns'] / 1.0E6
t1['failure'] = t1['http_resp_status'] >= 400
# edit this to increase/decrease window. Dont go lower than 1 second.
t1['window1'] = px.bin(t1['time_'], px.seconds(10))
t1['window2'] = px.bin(t1['time_'] + px.seconds(5), px.seconds(10))
window1_agg = t1.groupby(['service', 'window1']).agg(
  quantiles=('http_resp_latency_ms', px.quantiles),
)
window1_agg['p50'] = px.pluck_float64(window1_agg['quantiles'], 'p50')
window1_agg['p90'] = px.pluck_float64(window1_agg['quantiles'], 'p90')
window1_agg['p99'] = px.pluck_float64(window1_agg['quantiles'], 'p99')
window1_agg['time_'] = window1_agg['window1']
window1_agg = window1_agg.drop(['window1', 'quantiles'])
window = window1_agg[True]
px.display(window)
)pxl";

TEST_F(CompilerTest, BadDropQuery) {
  auto graph_or_s = compiler_.CompileToIR(kBadDropQuery, compiler_state_.get());
  ASSERT_OK(graph_or_s);

  MemorySinkIR* mem_sink;
  auto graph = graph_or_s.ConsumeValueOrDie();
  for (int64_t i : graph->dag().TopologicalSort()) {
    auto node = graph->Get(i);
    if (Match(node, MemorySink())) {
      mem_sink = static_cast<MemorySinkIR*>(node);
      break;
    }
  }
  ASSERT_NE(mem_sink, nullptr);
  Relation expected_relation(
      {types::STRING, types::FLOAT64, types::FLOAT64, types::FLOAT64, types::TIME64NS},
      {"service", "p50", "p90", "p99", "time_"});
  EXPECT_EQ(mem_sink->relation(), expected_relation);
  ASSERT_MATCH(mem_sink->parents()[0], Filter());
  FilterIR* filter = static_cast<FilterIR*>(mem_sink->parents()[0]);
  ASSERT_MATCH(filter->parents()[0], Map());
  MapIR* map = static_cast<MapIR*>(filter->parents()[0]);
  EXPECT_EQ(map->relation(), expected_relation);
}

constexpr char kDropWithoutListQuery[] = R"pxl(
import px
t1 = px.DataFrame(table='cpu', select=['cpu0', 'cpu1'])
t1 = t1.drop('cpu0')
px.display(t1)
)pxl";

TEST_F(CompilerTest, DropWithoutListQuery) {
  auto graph_or_s = compiler_.CompileToIR(kDropWithoutListQuery, compiler_state_.get());
  ASSERT_OK(graph_or_s);

  MemorySinkIR* mem_sink;
  auto graph = graph_or_s.ConsumeValueOrDie();
  for (int64_t i : graph->dag().TopologicalSort()) {
    auto node = graph->Get(i);
    if (Match(node, MemorySink())) {
      mem_sink = static_cast<MemorySinkIR*>(node);
      break;
    }
  }
  ASSERT_NE(mem_sink, nullptr);

  Relation expected_relation({types::FLOAT64}, {"cpu1"});
  ASSERT_EQ(mem_sink->relation(), expected_relation);
}

TEST_F(CompilerTest, AndExpressionFailsGracefully) {
  auto query =
      absl::StrJoin({"import px", "df = px.DataFrame('bar')",
                     "df[df['service'] != '' && px.asid() != 10]", "px.display(df, 'out')"},
                    "\n");
  auto ir_graph_or_s = compiler_.Compile(query, compiler_state_.get());
  ASSERT_NOT_OK(ir_graph_or_s);

  EXPECT_THAT(ir_graph_or_s.status(),
              HasCompilerError("SyntaxError: Expected expression after operator"));
}

// TODO(nserrino): PL-1578 Re-enable when "import px" append hack is removed from compiler.cc
TEST_F(CompilerTest, DISABLED_CommentOnlyCodeShouldFailGracefullly) {
  auto query = "# this is a comment";
  auto ir_graph_or_s = compiler_.Compile(query, compiler_state_.get());
  ASSERT_NOT_OK(ir_graph_or_s);

  EXPECT_THAT(ir_graph_or_s.status(), HasCompilerError("No runnable code found"));
}

constexpr char kMetadataNoDuplicatesQuery[] = R"pxl(
import px
t1 = px.DataFrame(table='cpu', start_time='-5s', select=['upid'])
t1.service_name = t1.ctx['service_name']
px.display(t1)
)pxl";

TEST_F(CompilerTest, MetadataNoDuplicateColumnsQuery) {
  auto graph_or_s = compiler_.CompileToIR(kMetadataNoDuplicatesQuery, compiler_state_.get());
  ASSERT_OK(graph_or_s);

  MemorySinkIR* mem_sink;
  auto graph = graph_or_s.ConsumeValueOrDie();
  for (int64_t i : graph->dag().TopologicalSort()) {
    auto node = graph->Get(i);
    if (Match(node, MemorySink())) {
      mem_sink = static_cast<MemorySinkIR*>(node);
      break;
    }
  }
  ASSERT_NE(mem_sink, nullptr);

  // ensure service_name is in relation but _attr_service_name is not
  Relation expected_relation({types::UINT128, types::STRING}, {"upid", "service_name"});
  ASSERT_EQ(mem_sink->relation(), expected_relation);
}

TEST_F(CompilerTest, UnusedOperatorsRemoved) {
  std::string query = absl::StrJoin(
      {
          "import px",
          "queryDF = px.DataFrame(table='http_table', select=['time_', 'upid', 'http_resp_status', "
          "'http_resp_latency_ns'], start_time='-1m')",
          "filter = queryDF[queryDF['http_resp_latency_ns'] < 1000000]",
          "drop = filter.drop(['http_resp_latency_ns', 'upid', 'http_resp_status'])",
          "unused_map = filter",
          "unused_map.http_resp_latency_ns = unused_map['http_resp_latency_ns'] * 2",
          "px.display(drop, 'out');",
      },

      "\n");
  auto plan_status = compiler_.CompileToIR(query, compiler_state_.get());
  VLOG(2) << plan_status.ToString();
  ASSERT_OK(plan_status);
  auto plan = plan_status.ConsumeValueOrDie();
  std::vector<IRNode*> mem_srcs = plan->FindNodesOfType(IRNodeType::kMemorySource);
  ASSERT_EQ(mem_srcs.size(), 1);
  MemorySourceIR* mem_src = static_cast<MemorySourceIR*>(mem_srcs[0]);

  EXPECT_EQ(mem_src->Children().size(), 1);

  OperatorIR* src_child = mem_src->Children()[0];
  ASSERT_MATCH(src_child, Filter());
  FilterIR* filter_src_child = static_cast<FilterIR*>(src_child);
  EXPECT_MATCH(filter_src_child->filter_expr(), LessThan(ColumnNode(), Int(1000000)));
  EXPECT_EQ(filter_src_child->Children().size(), 1);

  OperatorIR* filter_child1 = filter_src_child->Children()[0];
  ASSERT_MATCH(filter_child1, Map());
  EXPECT_EQ(filter_child1->Children().size(), 1);

  OperatorIR* filter_child_child = filter_child1->Children()[0];
  ASSERT_MATCH(filter_child_child, MemorySink());
  MemorySinkIR* mem_sink = static_cast<MemorySinkIR*>(filter_child_child);
  Relation expected_relation({types::TIME64NS}, {"time_"});
  EXPECT_EQ(mem_sink->relation(), expected_relation);
}

constexpr char kUndefinedFuncError[] = R"pxl(
import px
queryDF = px.DataFrame(table='cpu', select=['upid'])
queryDF.foo = queryDF.bar(queryDF.upid)
px.display(queryDF, 'map')
)pxl";

TEST_F(CompilerTest, UndefinedFuncError) {
  auto graph_or_s = compiler_.CompileToIR(kUndefinedFuncError, compiler_state_.get());
  ASSERT_NOT_OK(graph_or_s);

  EXPECT_THAT(graph_or_s.status(), HasCompilerError("dataframe has no method 'bar'"));
}

TEST_F(CompilerTest, TestUnaryOperators) {
  std::string query = absl::StrJoin(
      {
          "import px",
          "df = px.DataFrame(table='cpu', select=['cpu0'])",
          "df.foo = 1",
          "df.foo2 = ~df.foo",
          "df.bar = -df.cpu0",
          "df.bar2 = +df.bar",
          "df = df[not df.foo == df.bar]",
          "df.bool = df.foo != df.bar",
          "df.baz = not df.bool",
          "px.display(df)",
      },
      "\n");

  auto plan_status = compiler_.CompileToIR(query, compiler_state_.get());
  ASSERT_OK(plan_status);
}

// ENABLE and modify these tests once we know where rolling is getting merged into
constexpr char kRollingTimeStringQuery[] = R"pxl(
import px
t1 = px.DataFrame(table='http_events', select=['time_', 'remote_port'])
t1 = t1.rolling('3s')
px.display(t1)
)pxl";
TEST_F(CompilerTest, DISABLED_RollingTimeStringQuery) {
  auto graph_or_s = compiler_.CompileToIR(kRollingTimeStringQuery, compiler_state_.get());
  ASSERT_OK(graph_or_s);
  auto graph = graph_or_s.ConsumeValueOrDie();

  std::vector<IRNode*> rolling_nodes = graph->FindNodesOfType(IRNodeType::kRolling);
  ASSERT_EQ(rolling_nodes.size(), 1);
  auto rolling = static_cast<RollingIR*>(rolling_nodes[0]);

  ASSERT_EQ(rolling->window_col()->col_name(), "time_");
  ASSERT_MATCH(rolling->window_size(), Int());
  IntIR* window_size_int = static_cast<IntIR*>(rolling->window_size());
  ASSERT_EQ(window_size_int->val(),
            std::chrono::duration_cast<std::chrono::nanoseconds>(std::chrono::seconds(3)).count());
  Relation rolling_relation({types::TIME64NS, types::INT64}, {"time_", "remote_port"});
  EXPECT_EQ(rolling_relation, rolling->relation());
  // TODO(james): the rolling will likely get merged into some kind of Agg or MergedAgg so this test
  // will nechange.
}

constexpr char kRollingIntQuery[] = R"pxl(
import px
t1 = px.DataFrame(table='http_events', select=['time_', 'remote_port'])
t1 = t1.rolling(3000)
px.display(t1)
)pxl";
TEST_F(CompilerTest, DISABLED_RollingIntQuery) {
  auto graph_or_s = compiler_.CompileToIR(kRollingIntQuery, compiler_state_.get());
  ASSERT_OK(graph_or_s);
  auto graph = graph_or_s.ConsumeValueOrDie();

  std::vector<IRNode*> rolling_nodes = graph->FindNodesOfType(IRNodeType::kRolling);
  ASSERT_EQ(rolling_nodes.size(), 1);
  auto rolling = static_cast<RollingIR*>(rolling_nodes[0]);

  ASSERT_EQ(rolling->window_col()->col_name(), "time_");
  ASSERT_MATCH(rolling->window_size(), Int());
  IntIR* window_size_int = static_cast<IntIR*>(rolling->window_size());
  ASSERT_EQ(window_size_int->val(), 3000);
  Relation rolling_relation({types::TIME64NS, types::INT64}, {"time_", "remote_port"});
  EXPECT_EQ(rolling_relation, rolling->relation());
  // TODO(james): the rolling will likely get merged into some kind of Agg or MergedAgg so this test
  // will need to change.
}

constexpr char kRollingCompileTimeExprEvalQuery[] = R"pxl(
import px
t1 = px.DataFrame(table='http_events', select=['time_', 'remote_port'])
t1 = t1.rolling(1 + px.now())
px.display(t1)
)pxl";
TEST_F(CompilerTest, DISABLED_RollingCompileTimeExprEvalQuery) {
  auto graph_or_s = compiler_.CompileToIR(kRollingCompileTimeExprEvalQuery, compiler_state_.get());
  ASSERT_OK(graph_or_s);
  auto graph = graph_or_s.ConsumeValueOrDie();

  std::vector<IRNode*> rolling_nodes = graph->FindNodesOfType(IRNodeType::kRolling);
  ASSERT_EQ(rolling_nodes.size(), 1);
  auto rolling = static_cast<RollingIR*>(rolling_nodes[0]);

  ASSERT_EQ(rolling->window_col()->col_name(), "time_");
  ASSERT_MATCH(rolling->window_size(), Int());
  IntIR* window_size_int = static_cast<IntIR*>(rolling->window_size());
  ASSERT_EQ(window_size_int->val(), compiler_state_->time_now().val + 1);
  Relation rolling_relation({types::TIME64NS, types::INT64}, {"time_", "remote_port"});
  EXPECT_EQ(rolling_relation, rolling->relation());
  // TODO(james): the rolling will likely get merged into some kind of Agg or MergedAgg so this test
  // will need to change.
}

constexpr char kRollingNonTimeColumn[] = R"pxl(
import px
t1 = px.DataFrame(table='cpu', select=['cpu0'])
t1 = t1.rolling(1, on='cpu0')
px.display(t1)
)pxl";
TEST_F(CompilerTest, RollingNonTimeUnsupported) {
  auto graph_or_s = compiler_.CompileToIR(kRollingNonTimeColumn, compiler_state_.get());
  ASSERT_NOT_OK(graph_or_s);

  EXPECT_THAT(graph_or_s.status(),
              HasCompilerError("Windowing is only supported on time_ at the moment"));
}

const char* kFunctionOptimizationQuery = R"pxl(
import px
bytes_per_mb = 1024.0 * 1024.0
window_s = 10
node_col = 'node'
k8s_object = 'pod'
split_series_name = 'pod'
def format_stats_table(df, window_size):
    df.timestamp = px.bin(df.time_, px.seconds(window_size))
    return df

def format_process_table(df, window_size: int):
    df = format_stats_table(df, window_size)
    df[node_col] = df.ctx['node_name']
    df[k8s_object] = df.ctx['pod']

    # Convert bytes to MB.
    df.vsize_mb = df.vsize_bytes / bytes_per_mb
    df.rss_mb = df.rss_bytes / bytes_per_mb

    # Convert nanoseconds to milliseconds.
    df.cpu_utime_ms = df.cpu_utime_ns / 1.0E6
    df.cpu_ktime_ms = df.cpu_ktime_ns / 1.0E6
    return df

def filter_stats(df, node_filter, k8s_filter):
    df = df[px.contains(df[node_col], node_filter)]
    df = df[px.contains(df[k8s_object], k8s_filter)]
    return df

def filtered_process_stats(start_time: str, node_name: str, pod_name: px.Service):
    process_df = px.DataFrame(table='process_stats', start_time=start_time)
    process_df = format_process_table(process_df, window_s)
    process_df = filter_stats(process_df, node_name, pod_name)
    return process_df

def calculate_cpu(df, window_size):
    cpu_by_upid = df.groupby(['upid', k8s_object, 'timestamp']).agg(
        cpu_utime_ms_max=('cpu_utime_ms', px.max),
        cpu_utime_ms_min=('cpu_utime_ms', px.min),
        cpu_ktime_ms_max=('cpu_ktime_ms', px.max),
        cpu_ktime_ms_min=('cpu_ktime_ms', px.min),
        rss_mb=('rss_mb', px.mean),
    )

    cpu_by_upid.cpu_utime_ms = cpu_by_upid.cpu_utime_ms_max - cpu_by_upid.cpu_utime_ms_min
    cpu_by_upid.cpu_ktime_ms = cpu_by_upid.cpu_ktime_ms_max - cpu_by_upid.cpu_ktime_ms_min

    cpu_per_k8s = cpu_by_upid.groupby([k8s_object, 'timestamp']).agg(
        cpu_ktime_ms=('cpu_ktime_ms', px.sum),
        cpu_utime_ms=('cpu_utime_ms', px.sum),
        rss_mb=('rss_mb', px.mean),
    )

    # Convert window_size into the same units as cpu time.
    window_size_ms = window_size * 1.0E3
    # Finally, calculate total (kernel + user time)  percentage used over window.
    cpu_per_k8s.cpu_pct = (cpu_per_k8s.cpu_ktime_ms
                           + cpu_per_k8s.cpu_utime_ms) / window_size_ms * 100
    cpu_per_k8s['time_'] = cpu_per_k8s['timestamp']
    return cpu_per_k8s

def cpu_stats(start_time: str, node_name: str, pod_name: str):
    df = filtered_process_stats(start_time, node_name, pod_name)
    # Calculate the CPU usage per window.
    cpu_df = calculate_cpu(df, window_s)
    cpu_df[split_series_name] = cpu_df[k8s_object]
    return cpu_df['time_', split_series_name, 'cpu_pct', 'rss_mb']

def node_table(start_time: str, node_name: str, pod_name: str):
    df = filtered_process_stats(start_time, node_name, pod_name)
    nodes = df.groupby(node_col).agg(cc=(node_col, px.count))
    return nodes.drop('cc')
)pxl";

const char* kFuncToExecutePbStr = R"proto(
arg_values: {
  name: "start_time"
  value: "-30s"
}
arg_values: {
  name: "pod_name"
  value: ""
}
arg_values: {
  name: "node_name"
  value: ""
}
output_table_prefix: "foo"
)proto";

TEST_F(CompilerTest, multiple_def_calls_get_optimized) {
  ExecFuncs exec_funcs;
  FuncToExecute func;

  ASSERT_TRUE(google::protobuf::TextFormat::MergeFromString(kFuncToExecutePbStr, &func));

  // Push 3 times.
  func.set_func_name("cpu_stats");
  exec_funcs.push_back(func);
  exec_funcs.push_back(func);
  func.set_func_name("node_table");
  exec_funcs.push_back(func);

  auto graph_or_s =
      compiler_.CompileToIR(kFunctionOptimizationQuery, compiler_state_.get(), exec_funcs);
  ASSERT_OK(graph_or_s);

  auto graph = graph_or_s.ConsumeValueOrDie();
  auto mem_src_vec = graph->FindNodesThatMatch(MemorySource());
  ASSERT_EQ(mem_src_vec.size(), 1);
  auto mem_src = static_cast<MemorySourceIR*>(mem_src_vec[0]);
  EXPECT_EQ(mem_src->Children().size(), 1);

  for (auto child : mem_src->Children()) {
    ASSERT_MATCH(child, Map());
  }
  ASSERT_MATCH(mem_src->Children()[0], Map());
  ASSERT_EQ(graph->FindNodesThatMatch(MemorySink()).size(), 3);
}

constexpr char kPodNodeTypesQuery[] = R"pxl(
import px
def f(a: px.Pod, b: px.Node):
  return px.DataFrame('http_events')
)pxl";

TEST_F(CompilerTest, pod_and_node_types) {
  ExecFuncs exec_funcs;
  FuncToExecute f;
  f.set_func_name("f");
  f.set_output_table_prefix("f_out");
  auto a = f.add_arg_values();
  a->set_name("a");
  a->set_value("my_pod_name");
  auto b = f.add_arg_values();
  b->set_name("b");
  b->set_value("my_node_name");
  exec_funcs.push_back(f);

  auto graph_or_s = compiler_.CompileToIR(kPodNodeTypesQuery, compiler_state_.get(), exec_funcs);
  ASSERT_OK(graph_or_s);
}

constexpr char kCastQuery[] = R"pxl(
import px
df = px.DataFrame(table='process_stats', select=['vsize_bytes'])
df.vsize_bytes = px.Bytes(df.vsize_bytes)
px.display(df)
)pxl";

TEST_F(CompilerTest, casting) {
  auto graph_or_s = compiler_.CompileToIR(kCastQuery, compiler_state_.get());
  ASSERT_OK(graph_or_s);

  auto ir = graph_or_s.ConsumeValueOrDie();
  auto sinks = ir->FindNodesThatMatch(MemorySink());
  ASSERT_EQ(1, sinks.size());
  auto sink = static_cast<MemorySinkIR*>(sinks[0]);
  auto type_table = std::static_pointer_cast<TableType>(sink->resolved_type());
  EXPECT_TRUE(type_table->HasColumn("vsize_bytes"));
  auto col_type_or_s = type_table->GetColumnType("vsize_bytes");
  ASSERT_OK(col_type_or_s);
  EXPECT_PTR_VAL_EQ(ValueType::Create(types::INT64, types::ST_BYTES),
                    std::static_pointer_cast<ValueType>(col_type_or_s.ConsumeValueOrDie()));
}

constexpr char kBadCastQuery[] = R"pxl(
import px
df = px.DataFrame(table='process_stats', select=['vsize_bytes'])
df.vsize_bytes = px.Service(df.vsize_bytes)
px.display(df)
)pxl";

TEST_F(CompilerTest, bad_cast) {
  auto graph_or_s = compiler_.CompileToIR(kBadCastQuery, compiler_state_.get());
  ASSERT_NOT_OK(graph_or_s);

  EXPECT_THAT(graph_or_s.status(),
              HasCompilerError("Cannot cast from 'INT64' to 'STRING'. Only semantic type casts "
                               "are allowed."));
}

constexpr char kTimeIntCastQuery[] = R"pxl(
import px
df = px.DataFrame(table='sequences', select=['time_'])
df.time_ = px.Time(df.time_)
px.display(df)
)pxl";

TEST_F(CompilerTest, time_casting) {
  auto graph_or_s = compiler_.CompileToIR(kTimeIntCastQuery, compiler_state_.get());
  ASSERT_OK(graph_or_s);

  auto ir = graph_or_s.ConsumeValueOrDie();
  auto sinks = ir->FindNodesThatMatch(MemorySink());
  ASSERT_EQ(1, sinks.size());
  auto sink = static_cast<MemorySinkIR*>(sinks[0]);
  auto type_table = std::static_pointer_cast<TableType>(sink->resolved_type());
  EXPECT_TRUE(type_table->HasColumn("time_"));
  auto col_type_or_s = type_table->GetColumnType("time_");
  ASSERT_OK(col_type_or_s);
  // Time cast is currently a noop since Time is treated as a data type. We should move time to be a
  // semantic type and then update this test.
  EXPECT_PTR_VAL_EQ(ValueType::Create(types::TIME64NS, types::ST_NONE),
                    std::static_pointer_cast<ValueType>(col_type_or_s.ConsumeValueOrDie()));
}

constexpr char kMetadataSTQuery[] = R"pxl(
import px
df = px.DataFrame(table='process_stats', select=['upid'])
df.svc = df.ctx['service']
df.pod_name = df.ctx['pod']
df.node_name = df.ctx['node']
px.display(df)
)pxl";

TEST_F(CompilerTest, metadata_types) {
  auto graph_or_s = compiler_.CompileToIR(kMetadataSTQuery, compiler_state_.get());
  ASSERT_OK(graph_or_s);

  auto ir = graph_or_s.ConsumeValueOrDie();
  auto sinks = ir->FindNodesThatMatch(MemorySink());
  ASSERT_EQ(1, sinks.size());
  auto sink = static_cast<MemorySinkIR*>(sinks[0]);
  auto type_table = std::static_pointer_cast<TableType>(sink->resolved_type());

  EXPECT_TableHasColumnWithType(type_table, "svc",
                                ValueType::Create(types::STRING, types::ST_SERVICE_NAME));
  EXPECT_TableHasColumnWithType(type_table, "pod_name",
                                ValueType::Create(types::STRING, types::ST_POD_NAME));
  EXPECT_TableHasColumnWithType(type_table, "node_name",
                                ValueType::Create(types::STRING, types::ST_NODE_NAME));
}
}  // namespace compiler
}  // namespace planner
}  // namespace carnot
}  // namespace pl