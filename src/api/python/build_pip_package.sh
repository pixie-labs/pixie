#!/usr/bin/env bash

# Modified from original Tensorflow version.
# https://github.com/tensorflow/tensorflow/blob/bab0d14036efd0adcd4e48303d045cee3c342cb0/tensorflow/tools/pip_package/build_pip_package.sh
# This maintains the license and notice below:

# Copyright 2015 The TensorFlow Authors. All Rights Reserved.
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#     http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.
# ==============================================================================
set -e

source src/api/python/copy_python_srcs.sh

function is_absolute() {
  [[ "$1" = /* ]] || [[ "$1" =~ ^[a-zA-Z]:[/\\].* ]]
}

function real_path() {
  is_absolute "$1" && echo "$1" || echo "$PWD/${1#./}"
}

function build_wheel() {
  if [ $# -lt 2 ]; then
    echo "No src and dest dir provided"
    exit 1
  fi

  TMPDIR="$1"
  DEST="$2"

  # Before we leave the top-level directory, make sure we know how to
  # call python.
  if [[ -e tools/python_bin_path.sh ]]; then
    source tools/python_bin_path.sh
  fi

  pushd "${TMPDIR}" >/dev/null

  rm -f MANIFEST
  echo "$(date) : === Building wheel"
  "${PYTHON_BIN_PATH:-python}" setup.py sdist bdist_wheel >/dev/null
  mkdir -p "${DEST}"
  cp dist/* "${DEST}"
  popd >/dev/null
  echo "$(date) : === Output wheel file is in: ${DEST}"
}

function usage() {
  echo "Usage:"
  echo "$0 [--src srcdir] [--dst dstdir]"
  echo "$0 dstdir [options]"
  echo ""
  echo "    --src                 prepare sources in srcdir"
  echo "                              will use temporary dir if not specified"
  echo ""
  echo "    --dst                 build wheel in dstdir"
  echo "                              if dstdir is not set do not build, only prepare sources"
  echo ""
  exit 1
}

function prepare_python_src() {
  if [ $# -lt 1 ]; then
    echo "No destination dir provided"
    exit 1
  fi

  TMPDIR="${1%/}"
  mkdir -p "$TMPDIR"

  PYTHONPKG="src/api/python"
  TOOLPATH="src/api/python/build_pip_package"

  # Call the prep python repo because we want this to match.
  prepare_python_repo "${PYTHONPKG}" "${TOOLPATH}" "${TMPDIR}"

  PIXIEPKG="${TMPDIR}/pxapi"

  echo "$(date) : === Preparing protos in dir: ${TMPDIR}"
  # vizierapipb subpackage setup.
  cp -LR \
    "bazel-bin/${TOOLPATH}.runfiles/pl/src/api/public/vizierapipb" \
    "${PIXIEPKG}/vizierapipb"

  # cloudapipb subpackage setup.
  cp -LR \
    "bazel-bin/${TOOLPATH}.runfiles/pl/src/api/public/cloudapipb" \
    "${PIXIEPKG}/cloudapipb"

  # uuidpb subpackage setup.
  cp -LR \
    "bazel-bin/${TOOLPATH}.runfiles/pl/src/api/public/uuidpb" \
    "${PIXIEPKG}/uuidpb"

  # Each subpackage needs an __init__.py to be discovered by setuptools.
  touch "${PIXIEPKG}/vizierapipb/__init__.py"
  touch "${PIXIEPKG}/cloudapipb/__init__.py"
  touch "${PIXIEPKG}/uuidpb/__init__.py"

  # Rewrite the pb package import paths to the new directory structure.
  replace_pb_paths "${PIXIEPKG}"/**/*.py
  replace_pb_paths "${PIXIEPKG}"/*.py

  echo "$(date) : === Done preparing protos in dir: ${TMPDIR}"
}

function main() {
  SRCDIR=""
  DSTDIR=""
  CLEANSRC=1
  while true; do
    if [[ "$1" == "--help" ]]; then
      usage
      exit 1
    elif [[ "$1" == "--src" ]]; then
      shift
      SRCDIR=$(real_path "$1")
      CLEANSRC=0
    elif [[ "$1" == "--dst" ]]; then
      shift
      DSTDIR=$(real_path "$1")
    elif [[ -z "$1" ]]; then
      usage
      exit 1
    else
      DSTDIR=$(real_path "$1")
    fi
    shift

    if [[ -z "$1" ]]; then
      break
    fi
  done

  if [[ -z "$DSTDIR" ]] && [[ -z "$SRCDIR" ]]; then
    echo "No destination dir provided"
    usage
    exit 1
  fi

  if [[ -z "$SRCDIR" ]]; then
    # make temp srcdir if none set
    SRCDIR="$(mktemp -d -t tmp.XXXXXXXXXX)"
  fi

  echo "$(date) : === Preparing sources in dir: ${TMPDIR}"
  prepare_python_src "$SRCDIR"

  if [[ -z "$DSTDIR" ]]; then
    # only want to prepare sources
    exit
  fi

  build_wheel "$SRCDIR" "$DSTDIR"

  if [[ $CLEANSRC -ne 0 ]]; then
    rm -rf "${TMPDIR}"
  fi
}

main "$@"