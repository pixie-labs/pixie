<?php

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

final class ArcanistGoGenCheckerTestEngine extends ArcanistBaseGenCheckerTestEngine {
  private $goGenerateMap = array(
    'go-bindata' => '/(?<=-o=)(.*)(?=\.gen\.go)/',
    'mockgen' => '/(?<=-destination=)(.*)(?=\.gen\.go)/',
    'genny' => '/(?<=-out )(.*)(?=\.gen\.go)/',
  );

  // These go generate commands should be ignored.
  private $goGenerateIgnore = array('controller-gen');

  public function getEngineConfigurationName() {
    return 'go-gen-checker';
  }

  public function shouldEchoTestResults() {
    // This func/flag is badly named. Setting it to false, says that we don't render results
    // and that ArcanistConfigurationDrivenUnitTestEngine should do so instead.
    return false;
  }

  public function run() {
    $test_results = array();

    foreach ($this->getPaths() as $file) {
      $file_path = $this->getWorkingCopy()->getProjectRoot().DIRECTORY_SEPARATOR.$file;
      if (!file_exists($file_path)) {
        continue;
      }

      // Find if the .go file contains //go:generate.
      foreach (file($file_path) as $line_num => $line) {
        if (strpos($line, '//go:generate') !== false) {
          $command = preg_split('/\s+/', $line)[1];

          if (!array_key_exists($command, $this->goGenerateMap) && !in_array($command, $this->goGenerateIgnore)) {
            $res = new ArcanistUnitTestResult();
            $res->setName(get_class($this));
            $res->setResult(ArcanistUnitTestResult::RESULT_FAIL);
            $res->setUserData('go:generate command '.$command.' has not been added to goGenerateMap. Please add'.
              ' an entry in $goGenerateMap in tools/arc_addons/pixielabs/unit/ArcanistGoGenCheckerTestEngine.php, '.
              'where the key is '.$command.' and the value is a regex for the name of the generated output file.');
            $test_results[] = $res;
            break;
          }

          if (in_array($command, $this->goGenerateIgnore)) {
            break;
          }

          // Find the name of the .gen.go output file.
          $matches = array();
          preg_match($this->goGenerateMap[$command], $line, $matches);
          $gen_go_filename = substr($file, 0, strrpos($file, '/') + 1).$matches[0].'.gen.go';

          $test_results[] = $this->checkFile($file, $gen_go_filename, 'To regenerate, run "go generate" in the appropriate directory.');
          break;
        }
      }
    }

    return $test_results;
  }
}
