#pragma once
#include <memory>
#include <string>

#include <absl/container/flat_hash_map.h>
#include <absl/container/flat_hash_set.h>
#include <pypa/ast/ast.hh>

#include "src/carnot/planner/ast/ast_visitor.h"
#include "src/carnot/planner/ir/ast_utils.h"
#include "src/carnot/planner/ir/ir_nodes.h"
#include "src/carnot/planner/ir/pattern_match.h"

namespace pl {
namespace carnot {
namespace planner {
namespace compiler {

// Forward declaration necessary because FuncObjects are the methods in QLObject but
// are QLObjects themselves. Fully declared in "src/carnot/planner/objects/funcobject.h".
class FuncObject;

enum class QLObjectType {
  kMisc = 0,
  kDataframe,
  kFunction,
  kExpr,
  kList,
  kTuple,
  kNone,
  kPLModule,
  kMetadata,
  kVisualization,
  kType,
  kFlags,
};

class TypeDescriptor {
 public:
  constexpr TypeDescriptor(const std::string_view name, QLObjectType type)
      : name_(name), type_(type) {}
  const std::string_view& name() const { return name_; }
  const QLObjectType& type() const { return type_; }

 protected:
  const std::string_view name_;
  QLObjectType type_;
};

class QLObject;
// Alias for convenience.
using QLObjectPtr = std::shared_ptr<QLObject>;

class QLObject {
 public:
  virtual ~QLObject() = default;

  static StatusOr<QLObjectPtr> FromIRNode(IRNode* node, ASTVisitor* ast_visitor);

  /**
   * @brief Gets the Method with specified name.
   *
   * @param name the method to grab
   * @return ptr to the method. nullptr if not found.
   */
  StatusOr<std::shared_ptr<FuncObject>> GetMethod(std::string_view name) const {
    if (!methods_.contains(name)) {
      return CreateError("'$0' object has no attribute '$1'", type_descriptor_.name(), name);
    }
    return methods_.find(name)->second;
  }

  /**
   * @brief Gets the method that runs when the instantiated object is Called Directly
   * ie
   * ```
   * df = pl.DataFrame(...)
   * # dataframe object called
   * df()
   * ```
   *
   * @return StatusOr<std::shared_ptr<FuncObject>>
   */
  StatusOr<std::shared_ptr<FuncObject>> GetCallMethod() const {
    if (!HasMethod(kCallMethodName)) {
      return CreateError("'$0' object is not callable", type_descriptor_.name());
    }
    return GetMethod(kCallMethodName);
  }

  /**
   * @brief Get the method that runs when this object is called with a subscript.
   * ie
   * ```
   * df = pl.DataFrame(...)
   * a = dataframe[12 == 2]
   * ```
   *
   * @return StatusOr<std::shared_ptr<FuncObject>>
   */
  StatusOr<std::shared_ptr<FuncObject>> GetSubscriptMethod() const {
    if (!HasMethod(kSubscriptMethodName)) {
      return CreateError("'$0' object is not subscriptable", type_descriptor_.name());
    }
    return GetMethod(kSubscriptMethodName);
  }

  /**
   * @brief Returns whether object has a method with `name`
   *
   * @param name the string name of the method.
   * @return whether the object has the method.
   */
  bool HasMethod(std::string_view name) const { return methods_.find(name) != methods_.end(); }

  bool HasSubscriptMethod() const { return HasMethod(kSubscriptMethodName); }
  bool HasCallMethod() const { return HasMethod(kCallMethodName); }

  virtual bool CanAssignAttribute(std::string_view /*attr_name*/) const { return true; }
  Status AssignAttribute(std::string_view attr_name, QLObjectPtr object);

  StatusOr<std::shared_ptr<QLObject>> GetAttribute(const pypa::AstPtr& ast,
                                                   std::string_view attr_name) const;

  bool HasAttribute(std::string_view name) const {
    return HasNonMethodAttribute(name) || HasMethod(name);
  }

  /**
   * @brief Returns the name of all attributes that are not methods.
   *
   * @return absl::flat_hash_set<std::string> the set of all attributes.
   */
  absl::flat_hash_set<std::string> AllAttributes() const {
    absl::flat_hash_set<std::string> attrs;
    for (const auto& [k, v] : attributes_) {
      PL_UNUSED(v);
      attrs.insert(k);
    }
    return attrs;
  }

  const TypeDescriptor& type_descriptor() const { return type_descriptor_; }
  virtual std::string name() const { return std::string(type_descriptor_.name()); }
  QLObjectType type() const { return type_descriptor_.type(); }

  IRNode* node() const { return node_; }

  /**
   * @brief Returns whether this Object contains a valid node.
   *
   * @return the Node is not null.
   */
  bool HasNode() const { return node_ != nullptr; }

  bool HasAstPtr() const { return ast_ != nullptr; }

  /**
   * @brief Creates an error for this objects. Packages checks to make sure you have an ir node for
   * line,col error resporting. Defaults to standard error in case an ir node is nonexistant..
   *
   * @return Status
   */
  template <typename... Args>
  Status CreateError(Args... args) const {
    if (HasNode()) {
      return node_->CreateIRNodeError(args...);
    }
    if (HasAstPtr()) {
      return CreateAstError(ast_, args...);
    }
    return error::InvalidArgument(args...);
  }

  // Methods are all of the methods available. Exposed to make testing easier.
  const absl::flat_hash_map<std::string, std::shared_ptr<FuncObject>>& methods() const {
    return methods_;
  }

 protected:
  /**
   * @brief Construct a new QLObject. The type_descriptor must be a static member of the class.
   *
   *
   * @param type_descriptor the type descriptor
   * @param node the node to store in the QLObject. Can be null if not necessary for the
   * implementation of the QLObject.
   */
  QLObject(const TypeDescriptor& type_descriptor, IRNode* node, pypa::AstPtr ast,
           ASTVisitor* ast_visitor)
      : type_descriptor_(type_descriptor), node_(node), ast_(ast), ast_visitor_(ast_visitor) {}

  QLObject(const TypeDescriptor& type_descriptor, ASTVisitor* ast_visitor)
      : QLObject(type_descriptor, nullptr, nullptr, ast_visitor) {}

  QLObject(const TypeDescriptor& type_descriptor, IRNode* node, ASTVisitor* ast_visitor)
      : QLObject(type_descriptor, node, nullptr, ast_visitor) {}

  QLObject(const TypeDescriptor& type_descriptor, pypa::AstPtr ast, ASTVisitor* ast_visitor)
      : QLObject(type_descriptor, nullptr, ast, ast_visitor) {}

  /**
   * @brief Adds a method to the object. Used by QLObject derived classes to define methods.
   *
   * @param name name to reference for the method.
   * @param func_object the function object that represents the Method.
   */
  void AddMethod(const std::string& name, std::shared_ptr<FuncObject> func_object) {
    DCHECK(!HasMethod(name)) << "already exists.";
    methods_[name] = func_object;
  }

  /**
   * @brief Defines a call method for the object.
   *
   * @param func_object the func to set as the call method.
   */
  void AddCallMethod(std::shared_ptr<FuncObject> func_object) {
    AddMethod(kCallMethodName, func_object);
  }

  void AddSubscriptMethod(std::shared_ptr<FuncObject> func_object);

  virtual bool HasNonMethodAttribute(std::string_view name) const {
    return attributes_.contains(name);
  }

  /**
   * @brief nvi for GetAttributeImpl. Necessary so that dataframes, which have any value
   * as a possible attribute (because we don't know their column names) are able to override
   * and implement their own.
   *
   */
  virtual StatusOr<std::shared_ptr<QLObject>> GetAttributeImpl(const pypa::AstPtr& /*ast*/,
                                                               std::string_view attr_name) const {
    DCHECK(HasNonMethodAttribute(attr_name));
    return attributes_.at(attr_name);
  }

  // Reserved keyword for call.
  inline static constexpr char kCallMethodName[] = "__call__";
  inline static constexpr char kSubscriptMethodName[] = "__getitem__";
  ASTVisitor* ast_visitor() const { return ast_visitor_; }

 private:
  absl::flat_hash_map<std::string, std::shared_ptr<FuncObject>> methods_;
  absl::flat_hash_map<std::string, QLObjectPtr> attributes_;

  TypeDescriptor type_descriptor_;
  IRNode* node_ = nullptr;
  pypa::AstPtr ast_ = nullptr;
  ASTVisitor* ast_visitor_ = nullptr;
};

}  // namespace compiler
}  // namespace planner
}  // namespace carnot
}  // namespace pl