#!/usr/bin/env bash
set -e
pushd ./build
firebase deploy -P "{{ESTATE_FIREBASE_PROJECT_ID}}" --token "{{ESTATE_FIREBASE_DEPLOYMENT_KEY}}"
popd