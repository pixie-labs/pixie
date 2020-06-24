#pragma once

#include <gmock/gmock.h>
#include <memory>
#include <string>
#include <vector>

#include "src/carnot/exec/exec_node.h"

namespace pl {
namespace carnot {
namespace exec {

class MockExecNode : public ExecNode {
 public:
  MockExecNode() : ExecNode(ExecNodeType::kProcessingNode) {}
  explicit MockExecNode(const ExecNodeType& exec_node_type) : ExecNode(exec_node_type) {}

  MOCK_METHOD0(DebugStringImpl, std::string());
  MOCK_METHOD1(InitImpl, Status(const plan::Operator& plan_node));
  MOCK_METHOD1(PrepareImpl, Status(ExecState* exec_state));
  MOCK_METHOD1(OpenImpl, Status(ExecState* exec_state));
  MOCK_METHOD1(CloseImpl, Status(ExecState* exec_state));
  MOCK_METHOD1(GenerateNextImpl, Status(ExecState*));
  MOCK_METHOD3(ConsumeNextImpl, Status(ExecState*, const table_store::schema::RowBatch&, size_t));
};

class MockSourceNode : public SourceNode {
 public:
  explicit MockSourceNode(const table_store::schema::RowDescriptor& output_descriptor)
      : SourceNode() {
    output_descriptor_ = std::make_unique<table_store::schema::RowDescriptor>(output_descriptor);
  }

  MOCK_METHOD0(DebugStringImpl, std::string());
  MOCK_METHOD1(InitImpl, Status(const plan::Operator& plan_node));
  MOCK_METHOD1(PrepareImpl, Status(ExecState* exec_state));
  MOCK_METHOD1(OpenImpl, Status(ExecState* exec_state));
  MOCK_METHOD1(CloseImpl, Status(ExecState* exec_state));
  MOCK_METHOD1(GenerateNextImpl, Status(ExecState*));
  MOCK_METHOD3(ConsumeNextImpl, Status(ExecState*, const table_store::schema::RowBatch&, size_t));
  MOCK_METHOD0(NextBatchReady, bool());

  void SendEOS() { sent_eos_ = true; }
};

class MockSinkNode : public SinkNode {
 public:
  MockSinkNode() : SinkNode() {}

  MOCK_METHOD0(DebugStringImpl, std::string());
  MOCK_METHOD1(InitImpl, Status(const plan::Operator& plan_node));
  MOCK_METHOD1(PrepareImpl, Status(ExecState* exec_state));
  MOCK_METHOD1(OpenImpl, Status(ExecState* exec_state));
  MOCK_METHOD1(CloseImpl, Status(ExecState* exec_state));
  MOCK_METHOD1(GenerateNextImpl, Status(ExecState*));
  MOCK_METHOD3(ConsumeNextImpl, Status(ExecState*, const table_store::schema::RowBatch&, size_t));
};

}  // namespace exec
}  // namespace carnot
}  // namespace pl