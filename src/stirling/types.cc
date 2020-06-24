#include "src/stirling/types.h"

namespace pl {
namespace stirling {

stirlingpb::Element DataElement::ToProto() const {
  stirlingpb::Element element_proto;
  element_proto.set_name(std::string(name_));
  element_proto.set_type(type_);
  element_proto.set_ptype(ptype_);
  element_proto.set_desc(std::string(desc_));
  if (decoder_ != nullptr) {
    google::protobuf::Map<int64_t, std::string>* decoder = element_proto.mutable_decoder();
    *decoder = google::protobuf::Map<int64_t, std::string>(decoder_->begin(), decoder_->end());
  }
  return element_proto;
}

stirlingpb::TableSchema DataTableSchema::ToProto() const {
  stirlingpb::TableSchema table_schema_proto;

  // Populate the proto with Elements.
  for (auto element : elements_) {
    stirlingpb::Element* element_proto_ptr = table_schema_proto.add_elements();
    element_proto_ptr->MergeFrom(element.ToProto());
  }
  table_schema_proto.set_name(std::string(name_));
  table_schema_proto.set_tabletized(tabletized_);
  table_schema_proto.set_tabletization_key(tabletization_key_);

  return table_schema_proto;
}

}  // namespace stirling
}  // namespace pl