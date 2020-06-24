#pragma once

#ifdef __linux__

#include <filesystem>
#include <string>
#include <vector>

#include "src/common/base/base.h"

namespace pl {
namespace fs {

// These are wrappers around std::filesystem functions to convert error codes to Status.
// More functions should be added as needed.

Status CreateSymlink(const std::filesystem::path& target, const std::filesystem::path& link);

Status CreateDirectories(const std::filesystem::path& dir);

pl::StatusOr<std::filesystem::path> ReadSymlink(const std::filesystem::path& symlink);

/**
 * Joins multiple paths together.
 *
 * Note that unlike std::filesystem's operator/, all arguments are treated as relative paths.
 * For example, assuming:
 *   std::filesystem::path a = '/path/to/a';
 *   std::filesystem::path b = '/path/to/b';
 * JoinPath({a, b}) returns /path/to/a/path/to/b.
 * In contrast, a/b (std::filesystem::path's operator/) would return /path/to/b.
 */
std::filesystem::path JoinPath(const std::vector<const std::filesystem::path*>& paths);

// Designed for use in test code only.
Status CreateSymlinkIfNotExists(const std::filesystem::path& target,
                                const std::filesystem::path& link);

/**
 * Returns OK if the path exists. Returns error if does not exist, or failed to detect
 * (for example, because of lack of permission).
 */
Status Exists(const std::filesystem::path& path);
Status Copy(const std::filesystem::path& from, const std::filesystem::path& to,
            std::filesystem::copy_options options = std::filesystem::copy_options::none);
Status Remove(const std::filesystem::path& f);

StatusOr<std::filesystem::path> Absolute(const std::filesystem::path& path);

StatusOr<std::filesystem::path> Relative(const std::filesystem::path& path,
                                         const std::filesystem::path& base);

/**
 * Returns the relative path of the child relative to the parent, if parent is indeed a parent of
 * child.
 *
 * Returns error if parent isn't a parent of child.
 */
StatusOr<std::filesystem::path> GetChildRelPath(std::filesystem::path parent,
                                                std::filesystem::path child);

struct PathSplit {
  std::filesystem::path parent;
  std::filesystem::path child;
};

/**
 * Returns a list of pairs of paths, such that parent / child == path.
 * They are ordered from longest to shorted parents.
 */
std::vector<PathSplit> EnumerateParentPaths(const std::filesystem::path& path);

}  // namespace fs
}  // namespace pl

#endif