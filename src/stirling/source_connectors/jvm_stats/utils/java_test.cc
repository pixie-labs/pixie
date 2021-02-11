#include "src/stirling/source_connectors/jvm_stats/utils/java.h"

#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <filesystem>
#include <string>
#include <string_view>

#include <absl/strings/match.h>

#include "src/common/base/test_utils.h"
#include "src/common/exec/subprocess.h"
#include "src/common/testing/test_environment.h"

namespace pl {
namespace stirling {
namespace java {

// Tests that values are calculated correctly.
TEST(StatsTest, CommonValues) {
  std::vector<Stats::Stat> stat_vec = {
      {"sun.gc.collector.0.time", 1},
      {"sun.gc.collector.1.time", 1},
      {"sun.gc.generation.0.space.0.used", 1},
      {"sun.gc.generation.0.space.1.used", 1},
      {"sun.gc.generation.0.space.2.used", 1},
      {"sun.gc.generation.1.space.0.used", 1},
      {"sun.gc.generation.0.space.0.capacity", 1},
      {"sun.gc.generation.0.space.1.capacity", 1},
      {"sun.gc.generation.0.space.2.capacity", 1},
      {"sun.gc.generation.1.space.0.capacity", 1},
      {"sun.gc.generation.0.maxCapacity", 1},
      {"sun.gc.generation.1.maxCapacity", 1},
  };
  Stats stats(std::move(stat_vec));
  EXPECT_EQ(1, stats.YoungGCTimeNanos());
  EXPECT_EQ(1, stats.FullGCTimeNanos());
  EXPECT_EQ(4, stats.UsedHeapSizeBytes());
  EXPECT_EQ(4, stats.TotalHeapSizeBytes());
  EXPECT_EQ(2, stats.MaxHeapSizeBytes());
}

TEST(HsperfdataPathTest, ResultIsAsExpected) {
  const char kClassPath[] = "src/stirling/source_connectors/jvm_stats/testing/HelloWorld.jar";
  const std::string class_path = testing::TestFilePath(kClassPath);

  SubProcess hello_world;
  ASSERT_OK(hello_world.Start({"java", "-cp", class_path, "HelloWorld"}));

  // Give some time for the JVM process to start.
  std::string s;
  while (!absl::StartsWith(s, "Hello, World")) {
    ASSERT_OK(hello_world.Stdout(&s));
  }

  ASSERT_OK_AND_ASSIGN(auto hsperfdata_path, HsperfdataPath(hello_world.child_pid()));
  EXPECT_TRUE(std::filesystem::exists(hsperfdata_path));
  hello_world.Kill();
  EXPECT_EQ(9, hello_world.Wait()) << "Server should have been killed.";
}

}  // namespace java
}  // namespace stirling
}  // namespace pl