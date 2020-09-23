#pragma once

#include <absl/container/flat_hash_map.h>
#include <absl/container/flat_hash_set.h>

#include "src/common/system/proc_parser.h"
#include "src/shared/metadata/base_types.h"

namespace pl {
namespace stirling {

/**
 * Returns the list of processes from the proc filesystem.
 */
// TODO(yzhao): Consider moving this into src/common/system. Today that cannot be done because
// md::UPID is from src/shared/metadata, which already depends on src/common/system. Having this
// inside src/common/system would cause circular dependency.
absl::flat_hash_set<md::UPID> ListUPIDs(const std::filesystem::path& proc_path, uint32_t asid = 0);

/**
 * Keeps a list of UPIDs. Tracks newly-created and terminated processes each time an update is
 * provided, and updates its internal list of UPIDs.
 */
class ProcTracker : NotCopyMoveable {
 public:
  /**
   * Takes the current set of upids, and updates the internal state.
   * @param upids Current set of UPIDs.
   */
  void Update(absl::flat_hash_set<md::UPID> upids);

  /**
   * Returns all current upids, as set by last call to Update().
   */
  const auto& upids() const { return upids_; }

  /**
   * Returns new upids discovered by call to Update().
   */
  const auto& new_upids() const { return new_upids_; }

  /**
   * Returns upids that were deleted between last call the Update(), and the previous state.
   */
  const auto& deleted_upids() const { return deleted_upids_; }

 private:
  absl::flat_hash_set<md::UPID> upids_;
  absl::flat_hash_set<md::UPID> new_upids_;
  absl::flat_hash_set<md::UPID> deleted_upids_;
};

}  // namespace stirling
}  // namespace pl