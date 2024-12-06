#!/usr/bin/env bash
set -e
cd "$(dirname "$0")" || exit 1
cd .. || exit 1
source "./.preamble.sh"

rm -rf /tmp/secret-bundle
mkdir -p /tmp/secret-bundle/secrets
cd /tmp/secret-bundle

echo Adding files...
cp -fv ${ESTATE_KEYS_DIR}/firebase-${BUILD_TARGET}.json secrets/
cp -fv ${ESTATE_KEYS_DIR}/env.${BUILD_TARGET}.secrets.in.conf secrets/
cp -fv ${ESTATE_KEYS_DIR}/ip.${BUILD_TARGET}.conf secrets/

echo Making archive...
tar zcvf secrets.${BUILD_TARGET}.tgz secrets/*

clear
echo
echo In the ${BUILD_TARGET_U} GitHub Environment, create a GitHub Actions secret named:
echo  ESTATE_SECRETS
echo
echo Make its value this \(without surrounding whitespace\):
echo
cat secrets.${BUILD_TARGET}.tgz | base64 -w 0
echo