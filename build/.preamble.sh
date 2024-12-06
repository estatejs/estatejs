#!/usr/bin/env bash
source "./util/.functions.sh"

# Make sure the build environment is setup
source "./setup/estate.env"
ensure_env

BUILD_DATE=$(date +'%Y%m%d%H%M%S')
export BUILD_DATE
# shellcheck disable=SC2035
export ALL_AREAS=$(cd in || exit 1; stat -c '%F %n' * | sed -n '/^directory /s///p'; cd ..)

display_help() {
	echo "$(basename "${0}")"" <target> <type> <areas...>"
	echo "  target: local | test | production"
	echo "  type: debug | release"
	# shellcheck disable=SC2086
	echo "  area: none | all |" "$(join_by " | " ${ALL_AREAS})"
	exit 1
}

# default target is local
BUILD_TARGET_U="LOCAL"
BUILD_TARGET_P="Local"
BUILD_TARGET="local"
IS_LOCAL=1
BUILD_TARGET_FOUND=0
IS_PRODUCTION=0
IS_TEST=0

BUILD_TYPE_FOUND=0
# default type is debug
BUILD_TYPE_U="DEBUG"
BUILD_TYPE="Debug"
BUILD_TYPE_L="debug"
CPP_BUILD_TYPE="Debug"
CPP_BUILD_TYPE_L="debug"

ACCEPT_PUBLISH=0
FORCE_PUBLISH=0
IS_ALL_BUILD_AREA=0
BUILD_AREA=""
TRACE_ENABLED=0
FOLLOW_ENABLED=0
ESTATE_BUILD_CACHE_ENABLED=1

while [ -n "$1" ]; do
	case "$1" in
		local)
			if (( BUILD_TARGET_FOUND )); then
				error "extra build target"
				exit 1
			fi
			BUILD_TARGET_FOUND=1
			shift
		;;
		test)
			if (( BUILD_TARGET_FOUND )); then
				error "extra build target"
				exit 1
			fi
			BUILD_TARGET_U="TEST"
			BUILD_TARGET_P="Test"
			BUILD_TARGET="test"
			IS_TEST=1
			IS_LOCAL=0
			BUILD_TARGET_FOUND=1
			shift
		;;
		production)
			if (( BUILD_TARGET_FOUND )); then
				error "extra build target"
				exit 1
			fi
			BUILD_TARGET_U="PRODUCTION"
			BUILD_TARGET_P="Production"
			BUILD_TARGET="production"
			IS_PRODUCTION=1
			IS_LOCAL=0
			BUILD_TARGET_FOUND=1
			shift
		;;
		debug)
			if (( BUILD_TYPE_FOUND )); then
				error "extra build type"
				exit 1
			fi
			BUILD_TYPE_FOUND=1
			shift
		;;
		release)
			if (( BUILD_TYPE_FOUND )); then
				error "extra build type"
				exit 1
			fi
			BUILD_TYPE_U="RELEASE"
			BUILD_TYPE="Release"
			BUILD_TYPE_L="release"
			CPP_BUILD_TYPE="RelWithDebInfo"
			CPP_BUILD_TYPE_L="relwithdebinfo"
			BUILD_TYPE_FOUND=1
			shift
		;;
		-f)
			FOLLOW_ENABLED=1
			shift
		;;
		--no-cache)
			ESTATE_BUILD_CACHE_ENABLED=0
			shift
		;;
		--trace)
			TRACE_ENABLED=1
			shift
		;;
		--accept)
			ACCEPT_PUBLISH=1
			shift
		;;
		--force-publish)
			FORCE_PUBLISH=1
			shift
		;;
		all)
			IS_ALL_BUILD_AREA=1
			shift
		;;
		*)
			if [ -d "in/${1}" ]; then
				if (( IS_ALL_BUILD_AREA )); then
					error "Cannot specify additional areas to 'all'"
					exit 1
				fi
				if [ -z "${BUILD_AREA}" ]; then
					BUILD_AREA="${1}"
				else
					BUILD_AREA="${BUILD_AREA} ${1}"
				fi
			else
				error "Invalid area: ${1}"
				exit 1
			fi
			shift
		;;
	esac
done

if [ -z "${BUILD_AREA}" ]; then
	IS_ALL_BUILD_AREA=1
	BUILD_AREA="${ALL_AREAS}"
fi

export BUILD_TARGET_U
export BUILD_TARGET_P
export BUILD_TARGET
export IS_LOCAL
export IS_PRODUCTION
export IS_TEST

export BUILD_TYPE_U
export BUILD_TYPE
export BUILD_TYPE_L
export CPP_BUILD_TYPE
export CPP_BUILD_TYPE_L

export TRACE_ENABLED
export ACCEPT_PUBLISH
export FORCE_PUBLISH
export FOLLOW_ENABLED
export ESTATE_BUILD_CACHE_ENABLED

export IS_ALL_BUILD_AREA
export BUILD_AREA

export BUILD_INCLUDE_DIR="${PWD}/include"
export OUT_DIR="${PWD}/out"
export RENDER_DIR="${PWD}/out/${BUILD_TARGET}.${BUILD_TYPE_L}"
export RENDER_DIR_CACHE="${PWD}/out.cache/${BUILD_TARGET}.${BUILD_TYPE_L}"
export UTIL_DIR="${PWD}/util"
export LOCAL_DEPLOY_RUN_DIR="${RENDER_DIR}/.run"
export LOCAL_DEPLOY_STAGE_DIR="${RENDER_DIR}/.stage"
export CORE_COUNT=$(grep -c ^processor /proc/cpuinfo)