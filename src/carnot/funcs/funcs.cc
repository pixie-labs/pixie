#include "src/carnot/funcs/funcs.h"

#include "src/carnot/funcs/builtins/builtins.h"
#include "src/carnot/funcs/metadata/metadata_ops.h"

namespace pl {
namespace carnot {
namespace funcs {

void RegisterFuncsOrDie(udf::Registry* registry) {
  builtins::RegisterBuiltinsOrDie(registry);
  metadata::RegisterMetadataOpsOrDie(registry);
}

}  // namespace funcs
}  // namespace carnot
}  // namespace pl