#include <limits>
#include <map>
#include <memory>
#include <string>
#include <utility>
#include <vector>

#include <absl/strings/str_format.h>
#include "src/carnot/planner/compiler_state/registry_info.h"
#include "src/carnot/udfspb/udfs.pb.h"
#include "src/common/base/base.h"
#include "src/shared/types/proto/types.pb.h"

namespace pl {
namespace carnot {
namespace planner {

template <>
StatusOr<types::DataType> RegistryInfo::ResolveUDFSubType<types::DataType>(
    std::string name, std::vector<types::DataType> arg_types) {
  PL_ASSIGN_OR_RETURN(auto uda_or_udf, GetUDFExecType(name));
  switch (uda_or_udf) {
    case UDFExecType::kUDF:
      return GetUDFDataType(name, arg_types);
    case UDFExecType::kUDA:
      return GetUDADataType(name, arg_types);
    default:
      return error::Internal("Unsupported UDFExecType");
  }
}

template <>
StatusOr<types::SemanticType> RegistryInfo::ResolveUDFSubType<types::SemanticType>(
    std::string name, std::vector<types::SemanticType> arg_types) {
  auto type_or_s = semantic_rule_registry_.Lookup(name, arg_types);
  if (!type_or_s.ok()) {
    return types::ST_NONE;
  }
  return type_or_s.ConsumeValueOrDie();
}

Status RegistryInfo::Init(const udfspb::UDFInfo& info) {
  info_pb_ = info;
  for (const auto& uda : info.udas()) {
    std::vector<types::DataType> arg_types;
    arg_types.reserve(uda.update_arg_types_size());

    for (int64_t i = 0; i < uda.update_arg_types_size(); i++) {
      arg_types.push_back(uda.update_arg_types(i));
    }
    auto key = RegistryKey(uda.name(), arg_types);
    uda_map_[key] = uda.finalize_type();
    // Add uda to funcs_.
    if (funcs_.contains(uda.name())) {
      PL_ASSIGN_OR_RETURN(auto type, GetUDFExecType(uda.name()));
      DCHECK(UDFExecType::kUDA == type);
    }
    funcs_[uda.name()] = UDFExecType::kUDA;
  }

  for (const auto& udf : info.scalar_udfs()) {
    std::vector<types::DataType> arg_types;
    arg_types.reserve(udf.exec_arg_types_size());

    for (int64_t i = 0; i < udf.exec_arg_types_size(); i++) {
      arg_types.push_back(udf.exec_arg_types(i));
    }

    auto key = RegistryKey(udf.name(), arg_types);
    udf_map_[key] = udf.return_type();
    // Add udf to funcs_.
    if (funcs_.contains(udf.name())) {
      PL_ASSIGN_OR_RETURN(auto type, GetUDFExecType(udf.name()));
      DCHECK(UDFExecType::kUDF == type);
    }
    funcs_[udf.name()] = UDFExecType::kUDF;
  }

  for (const auto& udtf : info.udtfs()) {
    AddUDTF(udtf);
  }

  for (const auto& semantic_inf_rule : info.semantic_type_rules()) {
    AddSemanticInferenceRule(semantic_inf_rule);
  }

  return Status::OK();
}

StatusOr<UDFExecType> RegistryInfo::GetUDFExecType(std::string_view name) {
  if (!funcs_.contains(name)) {
    return error::InvalidArgument("Could not find function '$0'.", name);
  }
  return funcs_[name];
}

absl::flat_hash_set<std::string> RegistryInfo::func_names() const {
  absl::flat_hash_set<std::string> func_names;
  for (const auto& pair : funcs_) {
    func_names.insert(pair.first);
  }
  return func_names;
}

StatusOr<types::DataType> RegistryInfo::GetUDADataType(
    std::string name, std::vector<types::DataType> update_arg_types) {
  auto uda = uda_map_.find(RegistryKey(name, update_arg_types));
  if (uda == uda_map_.end()) {
    return error::InvalidArgument("Could not find UDA '$0' with update arg types [$1].", name,
                                  absl::StrJoin(update_arg_types, ","));
  }
  return uda->second;
}

StatusOr<types::DataType> RegistryInfo::GetUDFDataType(
    std::string name, std::vector<types::DataType> exec_arg_types) {
  auto udf = udf_map_.find(RegistryKey(name, exec_arg_types));
  if (udf == udf_map_.end()) {
    std::vector<std::string> arg_data_type_strs;
    for (const types::DataType& arg_data_type : exec_arg_types) {
      arg_data_type_strs.push_back(types::DataType_Name(arg_data_type));
    }
    return error::InvalidArgument("Could not find UDF '$0' with arguments [$1].", name,
                                  absl::StrJoin(arg_data_type_strs, ","));
  }
  return udf->second;
}

StatusOr<std::shared_ptr<ValueType>> RegistryInfo::ResolveUDFType(
    std::string name, const std::vector<std::shared_ptr<ValueType>>& arg_types) {
  std::vector<types::DataType> arg_data_types;
  std::vector<types::SemanticType> arg_semantic_types;
  for (const auto& basic_type : arg_types) {
    arg_data_types.push_back(basic_type->data_type());
    arg_semantic_types.push_back(basic_type->semantic_type());
  }
  PL_ASSIGN_OR_RETURN(auto out_data_type, ResolveUDFSubType(name, arg_data_types));
  PL_ASSIGN_OR_RETURN(auto out_semantic_type, ResolveUDFSubType(name, arg_semantic_types));
  return ValueType::Create(out_data_type, out_semantic_type);
}

void RegistryInfo::AddSemanticInferenceRule(const udfspb::SemanticInferenceRule& rule) {
  auto udfspb_type = rule.udf_exec_type();
  std::vector<types::SemanticType> arg_types;
  switch (udfspb_type) {
    case udfspb::SCALAR_UDF:
      for (const auto& type_int : rule.exec_arg_types()) {
        arg_types.push_back(types::SemanticType(type_int));
      }
      break;
    case udfspb::UDA:
      for (const auto& type_int : rule.update_arg_types()) {
        arg_types.push_back(types::SemanticType(type_int));
      }
      break;
    default:
      CHECK(false) << "Unknown udf type";
  }
  semantic_rule_registry_.Insert(rule.name(), arg_types, rule.output_type());
}

void SemanticRuleRegistry::Insert(std::string name, const ArgTypes& arg_types,
                                  types::SemanticType out_type) {
  auto it = map_.find(name);
  if (it == map_.end()) {
    map_[name] = TypeSet{};
  }
  map_[name].push_back(std::make_pair(arg_types, out_type));
}

StatusOr<types::SemanticType> SemanticRuleRegistry::Lookup(std::string name,
                                                           const ArgTypes& arg_types) const {
  auto it = map_.find(name);
  if (it == map_.end()) {
    return error::InvalidArgument("No semantic types registered for '$0'", name);
  }
  auto set = it->second;
  auto min_dist = std::numeric_limits<int>::max();
  types::SemanticType min_out_type;
  bool found = false;

  for (const auto& [arg_types_candidate, out_type] : set) {
    bool match;
    int dist;
    std::tie(match, dist) = MatchesCandidateAtDist(arg_types, arg_types_candidate);
    if (!match) {
      continue;
    }
    if (dist < min_dist) {
      min_dist = dist;
      min_out_type = out_type;
      found = true;
    }
  }

  if (!found) {
    return error::InvalidArgument("No semantic types match for '$0'", name);
  }
  return min_out_type;
}

std::pair<bool, int> SemanticRuleRegistry::MatchesCandidateAtDist(
    const ArgTypes& arg_types, const ArgTypes& arg_types_candidate) const {
  if (arg_types.size() != arg_types_candidate.size()) {
    return std::make_pair(false, 0);
  }
  int dist = 0;
  for (const auto& [idx, candidate_type] : Enumerate(arg_types_candidate)) {
    if (candidate_type != arg_types[idx] && candidate_type != types::ST_NONE) {
      return std::make_pair(false, 0);
    }
    if (candidate_type != arg_types[idx] && candidate_type == types::ST_NONE) {
      dist++;
    }
  }
  return std::make_pair(true, dist);
}

}  // namespace planner
}  // namespace carnot
}  // namespace pl