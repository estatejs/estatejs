#!/usr/bin/env bash
set -e
cd "$(dirname "$0")"

if [ "${#}" == 1 ] && [ "${1}" == "--ci" ]; then
    set -x
    [ -z "${BUILD_TARGET}" ] && echo "Missing BUILD_TARGET" && exit 1
    [ -z "${IS_PRODUCTION}" ] && echo "Missing IS_PRODUCTION" && exit 1
    [ -z "${IS_TEST}" ] && echo "Missing IS_TEST" && exit 1
    [ -z "${IS_LOCAL}" ] && echo "Missing IS_LOCAL" && exit 1
    [ -z "${BUILD_TYPE_L}" ] && echo "Missing BUILD_TYPE_L" && exit 1
else
    if [ ! "${#}" == 2 ]; then
        echo "error: Invalid options specified"
        echo $(basename ${0})" --ci | <target> <type>"
        echo "  target: local | test | production"
        echo "  type: debug | release"
        exit 1
    fi

    IS_LOCAL=0
    IS_PRODUCTION=0
    IS_TEST=0

    if [ "${1}" == "production" ]; then
        export BUILD_TARGET="production"
        IS_PRODUCTION=1
    elif [ "${1}" == "test" ]; then
        export BUILD_TARGET="test"
        IS_TEST=1
    elif [ "${1}" == "local" ]; then
        export BUILD_TARGET="local"
        IS_LOCAL=1
    else
        echo "error: Invalid build target \"${1}\""
        exit 1
    fi

    export IS_LOCAL
    export IS_PRODUCTION
    export IS_TEST

    if [ "${2}" == "debug" ]; then
        export BUILD_TYPE_L="debug"
    elif [ "${2}" == "release" ]; then
        export BUILD_TYPE_L="release"
    else
        echo "error: Invalid build type \"${2}\""
        exit 1
    fi
fi

if [ ! -f "${ESTATE_FRAMEWORK_DIR}/client/config.${BUILD_TARGET}.${BUILD_TYPE_L}.stamp" ]; then
    echo "error: Must render client configuration first"
    exit 1
fi

if [ ! -d "${ESTATE_FRAMEWORK_DIR}/client/dist" ]; then
    echo "error: Must build client first"
    exit 1
fi

if [ ! -f "./config.${BUILD_TARGET}.${BUILD_TYPE_L}.stamp" ]; then
    echo "error: Must render the configuration first"
    exit 1
fi

if [ "${BUILD_TYPE_L}" == "debug" ]; then
    export NODE_ENV="development"
    SOURCE_MAP=true
    DECLARATION=true
elif [ "${BUILD_TYPE_L}" == "release" ]; then
    export NODE_ENV="production"
    SOURCE_MAP=true
    DECLARATION=true
fi

echo "Copying client .d.ts files..."
cp "${ESTATE_FRAMEWORK_DIR}/client/dist/esm/public/model-types.d.ts" "src/templates/runtime/"
cp "${ESTATE_FRAMEWORK_DIR}/client/dist/esm/public/symbol.d.ts" "src/templates/runtime/"
cp "${ESTATE_FRAMEWORK_DIR}/client/dist/esm/public/uuid.d.ts" "src/templates/runtime/"

echo "Building tools for the ${BUILD_TARGET} environment in ${BUILD_TYPE_L} mode"

DIST_DIR=$(readlink -f "./dist")

rm -rf "${DIST_DIR}"

echo "Compiling TypeScript..."
./node_modules/.bin/tsc --outDir "${DIST_DIR}" --sourceMap ${SOURCE_MAP} --declaration ${DECLARATION}

echo "Creating npm package..."
DIST_DIR=${DIST_DIR} node ./setup-npm-package.js

echo "Copying README.md..."
cp "${ESTATE_FRAMEWORK_DIR}/README.md" "${DIST_DIR}/"

echo "Copying templates..."
mkdir -p "${DIST_DIR}/templates"
cp -r src/templates/* "${DIST_DIR}/templates/"

echo "Compressing and copying tutorial..."
pushd "${ESTATE_EXAMPLE_TUTORIAL_DIR}" > /dev/null
zip -9 -r "${DIST_DIR}/tutorial.zip" worker/ client/ > /dev/null
popd > /dev/null

echo "Compressing and copying example..."
pushd "${ESTATE_EXERCISE_TRACKER_DIR}" > /dev/null
zip -9 -r "${DIST_DIR}/example.zip" worker/ client/ > /dev/null
popd > /dev/null

echo "Updating permissions on index.js..."
chmod +x "${DIST_DIR}/index.js"