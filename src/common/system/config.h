#pragma once

#include <filesystem>
#include <memory>
#include <string>

#include "src/common/base/base.h"

namespace pl {
namespace system {

/**
 * This interface provides access to global system config.
 */
class Config : public NotCopyable {
 public:
  /**
   * Create an OS specific SystemConfig instance.
   * @return const reference to SystemConfig.
   */
  static const Config& GetInstance();

  virtual ~Config() {}

  /**
   * Checks if system config information is available.
   * @return true if system config is available
   */
  virtual bool HasConfig() const = 0;

  /**
   * Get the page size in the kernel.
   * @return page size in bytes.
   */
  virtual int64_t PageSize() const = 0;

  /**
   * Get the Kernel ticks per second.
   * @return int kernel ticks per second.
   */
  virtual int64_t KernelTicksPerSecond() const = 0;

  /**
   * If recording nsecs in your bt file, this function can be used to find the offset for
   * convert the result into realtime.
   */
  virtual uint64_t ClockRealTimeOffset() const = 0;

  /**
   * Get the sysfs path.
   */
  virtual const std::filesystem::path& sysfs_path() const = 0;

  /**
   * Get the host root path.
   */
  virtual const std::filesystem::path& host_path() const = 0;

  /**
   * Get the proc path.
   */
  virtual const std::filesystem::path& proc_path() const = 0;

 protected:
  Config() {}
};

}  // namespace system
}  // namespace pl