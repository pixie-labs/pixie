/*
 * Copyright 2018- The Pixie Authors.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include <gtest/gtest_prod.h>

#include <memory>
#include <string>
#include <utility>
#include <vector>

#include "src/common/base/base.h"
#include "src/common/system/system.h"
#include "src/shared/metadata/k8s_objects.h"

namespace px {
namespace md {

/**
 * CGroupMetadataReader is responsible for reading metadata such as process info from
 * sys/fs and proc.
 */
class CGroupMetadataReader : public NotCopyable {
 public:
  CGroupMetadataReader() = delete;
  virtual ~CGroupMetadataReader() = default;

  explicit CGroupMetadataReader(const system::Config& cfg);

  /**
   * ReadPIDList reads pids for a container running as part of a given pod.
   *
   * Note: that since this function contains inherent races with the system state and can return
   * errors when files fail to read because they have been deleted while the read was in progress.
   */
  virtual Status ReadPIDs(PodQOSClass qos_class, std::string_view pod_id,
                          std::string_view container_id, ContainerType container_type,
                          absl::flat_hash_set<uint32_t>* pid_set) const;

 private:
  void InitPathTemplates(std::string_view sysfs_path);

  std::string CGroupPodDirPath(PodQOSClass qos_class, std::string_view pod_id) const;

  std::string CGroupProcFilePath(PodQOSClass qos_class, std::string_view pod_id,
                                 std::string_view container_id, ContainerType container_type) const;

  std::string cgroup_kubepod_guaranteed_path_template_;
  std::string cgroup_kubepod_besteffort_path_template_;
  std::string cgroup_kubepod_burstable_path_template_;
  std::string container_template_;
  bool cgroup_kubepod_convert_dashes_;

  std::string proc_stat_path_template_;
  std::string proc_cmdline_path_template_;

  int64_t ns_per_kernel_tick_;
  int64_t clock_realtime_offset_;

  FRIEND_TEST(CGroupMetadataReaderTest, cgroup_pod_dir_path);
  FRIEND_TEST(CGroupMetadataReaderTest, cgroup_proc_file_path);
  FRIEND_TEST(CGroupMetadataReaderTest, cgroup_proc_file_path_alternate);
};

}  // namespace md
}  // namespace px
