#pragma once

#include <string>
// Most of this file is borrowed/inspired by envoy.
// Profiling support is provided in the release tcmalloc, but not in the library
// that supplies the debug tcmalloc. So all the profiling code must be ifdef'd
// on PROFILER_AVAILABLE which is dependent on those two settings.
#if defined(TCMALLOC) && !defined(PL_MEMORY_DEBUG_ENABLED)
#define PROFILER_AVAILABLE
#endif

namespace pl {
namespace profiler {

class CPU {
 public:
  /**
   * Returns if the profiler is available or not.
   * @return bool
   */
  static bool ProfilerAvailable();

  /*
   * Start profiler and store in the passed in path.
   * @return bool whether the call to start the profiler succeeded.
   */
  static bool StartProfiler(const std::string& output_path);

  /**
   * Stop the profiler.
   */
  static void StopProfiler();
};

class Heap {
 public:
  /**
   * Returns if the heap profiler is available or not.
   * @return bool
   */
  static bool ProfilerAvailable();

  /**
   * Check to see if profiler has been started.
   * @return bool true if profiler is started.
   */
  static bool IsProfilerStarted();

  /**
   * Start profiler and store in the passed in path.
   * @return bool whether the call to start the profiler succeeded.
   */
  static bool StartProfiler(const std::string& output_path);

  /**
   * Stop the profiler.
   * @return bool if the profiler was stopped and file written.
   */
  static bool StopProfiler();

 private:
  static void ForceLink();
};

}  // namespace profiler
}  // namespace pl