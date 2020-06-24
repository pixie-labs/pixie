#pragma once
#include <memory>
#include <string>

#include "src/carnot/planner/objects/funcobject.h"
#include "src/carnot/planner/objects/qlobject.h"

namespace pl {
namespace carnot {
namespace planner {
namespace compiler {

class MetadataObject : public QLObject {
 public:
  static constexpr TypeDescriptor MetadataTypeDescriptor = {
      /* name */ "metadata",
      /* type */ QLObjectType::kMetadata,
  };
  static StatusOr<std::shared_ptr<MetadataObject>> Create(OperatorIR* op, ASTVisitor* ast_visitor);

 protected:
  explicit MetadataObject(OperatorIR* op, ASTVisitor* ast_visitor)
      : QLObject(MetadataTypeDescriptor, ast_visitor), op_(op) {}

  Status Init();

  StatusOr<QLObjectPtr> SubscriptHandler(const pypa::AstPtr& ast, const ParsedArgs& args);

  OperatorIR* op() const { return op_; }

 private:
  OperatorIR* op_;
};

}  // namespace compiler
}  // namespace planner
}  // namespace carnot
}  // namespace pl