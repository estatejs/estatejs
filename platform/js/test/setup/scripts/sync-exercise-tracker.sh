#!/usr/bin/env bash
set -ex
cd $(dirname "${0}")
cd ..

# Copy the example's worker code and use that as a test worker backend
rm -rf ../project/workers/exercise-tracker > /dev/null
mkdir -p ../project/workers/exercise-tracker
cp ${ESTATE_EXERCISE_TRACKER_DIR}/worker/index.* ../project/workers/exercise-tracker/ > /dev/null