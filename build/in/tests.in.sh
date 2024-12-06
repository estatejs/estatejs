#!/usr/bin/env bash
set -e

pushd "{{ESTATE_PLATFORM_DIR}}/js/test/project/client/scripts" > /dev/null

echo "Exercise Tracker (Node) test"
bash ./test-exercise-tracker.sh

echo "Method and Serialization (Node & Browser) test"
bash ./test-method-and-serialization.sh

echo "Memory Cap test"
bash ./test-memory-cap.sh

popd > /dev/null

STAMP=$(date -Is)
echo "${STAMP}" > "{{RENDER_DIR}}/test.stamp"
echo "${STAMP}" > "{{RENDER_DIR}}/client/build/test.stamp"
echo "${STAMP}" > "{{RENDER_DIR}}/tools/build/test.stamp"
