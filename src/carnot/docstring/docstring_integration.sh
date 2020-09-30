#!/bin/bash
# --- begin runfiles.bash initialization ---
# Copy-pasted from Bazel's Bash runfiles library (tools/bash/runfiles/runfiles.bash).
set -euo pipefail
if [[ ! -d "${RUNFILES_DIR:-/dev/null}" && ! -f "${RUNFILES_MANIFEST_FILE:-/dev/null}" ]]; then
  if [[ -f "$0.runfiles_manifest" ]]; then
    export RUNFILES_MANIFEST_FILE="$0.runfiles_manifest"
  elif [[ -f "$0.runfiles/MANIFEST" ]]; then
    export RUNFILES_MANIFEST_FILE="$0.runfiles/MANIFEST"
  elif [[ -f "$0.runfiles/bazel_tools/tools/bash/runfiles/runfiles.bash" ]]; then
    export RUNFILES_DIR="$0.runfiles"
  fi
fi
if [[ -f "${RUNFILES_DIR:-/dev/null}/bazel_tools/tools/bash/runfiles/runfiles.bash" ]]; then
  # shellcheck disable=1090
  source "${RUNFILES_DIR}/bazel_tools/tools/bash/runfiles/runfiles.bash"
elif [[ -f "${RUNFILES_MANIFEST_FILE:-/dev/null}" ]]; then
  # shellcheck disable=1090
  source "$(grep -m1 "^bazel_tools/tools/bash/runfiles/runfiles.bash " \
            "$RUNFILES_MANIFEST_FILE" | cut -d ' ' -f 2-)"
else
  echo >&2 "ERROR: cannot find @bazel_tools//tools/bash/runfiles:runfiles.bash"
  exit 1
fi
# --- end runfiles.bash initialization ---

# -- begin loading the docstring binaries ---
extractor=$(rlocation "pl/src/carnot/planner/docs/doc_extractor")
if [[ ! -f "${extractor:-}" ]]; then
  echo >&2 "ERROR: could not find the doc_extractor binary"
  exit 1
fi
parser=$(rlocation "pl/src/carnot/docstring/docstring_/docstring")
if [[ ! -f "${parser:-}" ]]; then
  echo >&2 "ERROR: could not find the docstring binary parser"
  exit 1
fi
# -- end loading the docstring binaries ---

# Check to make sure the output JSON is specified.
if [ $# -eq 0 ]; then
  echo "Output JSON file not specified."
  exit 1
fi

raw_docstring_pb=./input.pb.txt
echo "Extracting the raw docs to a temporary file"
${extractor} --output_file ${raw_docstring_pb}
echo "Parsing the raw docs into '$1'"
${parser} --input_doc_pb ${raw_docstring_pb} --output_json "$1"
echo "Parsing complete"