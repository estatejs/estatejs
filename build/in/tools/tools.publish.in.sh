#!/usr/bin/env bash
if [ -n "${DRY_RUN}" ]; then
  set -e
  npm publish --dry-run
else
  set -ex
  npm publish
fi