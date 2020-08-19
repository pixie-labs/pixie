#include "src/stirling/utils/proc_tracker.h"

#include <utility>

#include "src/common/system/proc_parser.h"

namespace pl {
namespace stirling {

absl::flat_hash_set<md::UPID> ListUPIDs(const std::filesystem::path& proc_path, uint32_t asid) {
  absl::flat_hash_set<md::UPID> pids;
  for (const auto& p : std::filesystem::directory_iterator(proc_path)) {
    uint32_t pid = 0;
    if (!absl::SimpleAtoi(p.path().filename().string(), &pid)) {
      continue;
    }
    pids.emplace(asid, pid, system::GetPIDStartTimeTicks(p.path()));
  }
  return pids;
}

void ProcTracker::Update(absl::flat_hash_set<md::UPID> upids) {
  new_upids_.clear();
  for (const auto& upid : upids) {
    auto iter = upids_.find(upid);
    if (iter != upids_.end()) {
      upids_.erase(iter);
      continue;
    }
    new_upids_.emplace(upid);
  }
  deleted_upids_ = std::move(upids_);
  upids_ = std::move(upids);
}

}  // namespace stirling
}  // namespace pl