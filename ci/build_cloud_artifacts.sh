#!/bin/bash -ex

# shellcheck source=ci/artifact.sh
. "$(dirname "$0")/artifact.sh"


create_artifact "cloud" "skaffold/skaffold_cloud.yaml"