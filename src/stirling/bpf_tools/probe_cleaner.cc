#include <fcntl.h>

#include <fstream>
#include <sstream>
#include <string>
#include <utility>

#include "src/common/base/base.h"
#include "src/stirling/bpf_tools/probe_cleaner.h"

namespace pl {
namespace stirling {
namespace utils {

const char kAttachedKProbesFile[] = "/sys/kernel/debug/tracing/kprobe_events";
const char kAttachedUProbesFile[] = "/sys/kernel/debug/tracing/uprobe_events";

Status SearchForAttachedProbes(const char* file_path, std::string_view marker,
                               std::vector<std::string>* leaked_probes) {
  std::ifstream infile(file_path);
  if (!infile.good()) {
    return error::Internal("Failed to open file for reading: $0", file_path);
  }
  std::string line;
  while (std::getline(infile, line)) {
    if (absl::StrContains(line, marker)) {
      std::vector<std::string> split = absl::StrSplit(line, ' ');
      if (split.size() != 2) {
        return error::Internal("Unexpected format when reading file: $0", file_path);
      }

      std::string probe = std::move(split[0]);

      // Note that a probe looks like the following:
      //     p:kprobes/your_favorite_probe_name_here __x64_sys_connect
      // Perform a quick (but not thorough) sanity check that we have the right format.
      if ((probe[0] != 'p' && probe[0] != 'r') || probe[1] != ':') {
        return error::Internal("Unexpected probe string: $0", probe);
      }

      leaked_probes->push_back(std::move(probe));
    }
  }
  return Status::OK();
}

Status RemoveProbes(const char* file_path, std::vector<std::string> probes) {
  // Unfortunately std::ofstream doesn't properly append to /sys/kernel/debug/tracing/kprobe_events.
  // It appears related to its use of fopen() instead of open().
  // So doing the write in old-school C-style.
  int fd = open(file_path, O_WRONLY | O_APPEND, 0);
  if (fd < 0) {
    return error::Internal("Failed to open file for writing: $0", file_path);
  }

  std::vector<std::string> errors;
  for (auto& probe : probes) {
    // Here we modify first character, which is normally 'p' or 'r' to '-'.
    // This indicates that the probe should be removed.
    probe[0] = '-';
    VLOG(1) << absl::Substitute("Writing $0", probe);

    if (write(fd, probe.data(), probe.size()) < 0) {
      return error::System("Failed to write to file: $0 [errno=$1 message=$2]", file_path, errno,
                           std::strerror(errno));
    }
    // Note that even if write succeeds, it doesn't confirm that the probe was properly removed.
    // We can only confirm that we wrote to the file.
  }

  close(fd);

  return Status::OK();
}

namespace {

Status CleanProbesFromSysFile(const char* file_path, std::string_view marker) {
  LOG(INFO) << absl::Substitute("Cleaning probes from $0 with the following marker: $1", file_path,
                                marker);

  std::vector<std::string> leaked_probes;
  PL_RETURN_IF_ERROR(SearchForAttachedProbes(file_path, marker, &leaked_probes));
  PL_RETURN_IF_ERROR(RemoveProbes(file_path, leaked_probes));

  std::vector<std::string> leaked_probes_after;
  PL_RETURN_IF_ERROR(SearchForAttachedProbes(file_path, marker, &leaked_probes_after));
  if (!leaked_probes_after.empty()) {
    return error::Internal("Wasn't able to remove all probes. Initial count=$0, final count=$1",
                           leaked_probes.size(), leaked_probes_after.size());
  }
  LOG(INFO) << absl::Substitute("Cleaned up $0 probes", leaked_probes.size());

  return Status::OK();
}

}  // namespace

Status CleanProbes(std::string_view marker) {
  PL_RETURN_IF_ERROR(CleanProbesFromSysFile(kAttachedKProbesFile, marker));
  PL_RETURN_IF_ERROR(CleanProbesFromSysFile(kAttachedUProbesFile, marker));
  return Status::OK();
}

}  // namespace utils
}  // namespace stirling
}  // namespace pl