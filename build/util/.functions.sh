#!/usr/bin/env bash
function join_by {
  local d=$1
  shift
  local f=$1
  shift
  printf %s "$f" "${@/#/$d}"
}

export -f join_by

function list_includes {
  local list="$1"
  local item="$2"
  if [[ $list =~ (^|[[:space:]])"$item"($|[[:space:]]) ]] ; then
    return 0
  else
    return 1
  fi
}

export -f list_includes

function info {
  echo -e "${*}"
}
export -f info

function info_n {
  echo -n -e "${*}"
}
export -f info_n

function error {
  echo -e "\e[1;31m${*}\e[0m"
}
export -f error

function success {
  echo -e "\e[1;32m${*}\e[0m"
}
export -f success
function success_n {
  echo -n -e "\e[1;32m${*}\e[0m"
}
export -f success_n

function dry_run_echo {
  echo -e "\e[1;96m${*}\e[0m"
}
export -f dry_run_echo

function warning {
  echo -e "\e[1;33m${*}\e[0m"
}
export -f warning
function warning_n {
  echo -n -e "\e[1;33m${*}\e[0m"
}
export -f warning_n

function _ensure_env_var {
  if [ -z "${!1}" ]; then
    error "Missing ${1} environment variable"
    exit 1
  fi
  if [ ! -d "${!1}" ]; then
    if [ "${2}" != "missing-dir-ok" ]; then
      error "Directory '"${!1}"' pointed to by ${1} does not exist"
      exit 1
    fi
  fi
}

function ensure_env {
  _ensure_env_var ESTATE_ROOT_DIR
  _ensure_env_var ESTATE_PLATFORM_DIR
  _ensure_env_var ESTATE_NATIVE_DEPS_DIR missing-dir-ok
  _ensure_env_var ESTATE_BUILD_DIR
  _ensure_env_var ESTATE_KEYS_DIR
  _ensure_env_var ESTATE_FRAMEWORK_DIR
  _ensure_env_var ESTATE_DOCSITE_DIR
  _ensure_env_var ESTATE_EXERCISE_TRACKER_DIR
  _ensure_env_var ESTATE_EXAMPLE_TUTORIAL_DIR
}
export -f ensure_env

function ensure_build_target_is_branch {
  if [ ${#} == 0 ]; then
    error "Syntax error: ${0} <from> <build_target> [--ci]"
    exit 1
  fi
  if [ ${#} -lt 2 ] || [ ${#} -gt 3  ]; then
    error "${1} <build_target> [--ci]"
    exit 1
  fi
  BUILD_TARGET="${2}"
  local branch
  branch=$(git rev-parse --abbrev-ref HEAD)
  if [ "${branch}" != "${BUILD_TARGET}" ]; then
    error "Must switch to the ${BUILD_TARGET} branch."
    exit 1
  fi
  export BUILD_TARGET
  CONTINUOUS_INTEGRATION=0
  if [ "${3}" == "--ci" ]; then
    CONTINUOUS_INTEGRATION=1
  fi
  export CONTINUOUS_INTEGRATION
}
export -f ensure_build_target_is_branch

_has_updated=0
function apt_update_once {
	if (( !_has_updated )); then
		echo -n "Updating apt: "
    sudo apt-get update > /dev/null
		_has_updated=1
    success "Ok"
	fi	
}
export -f apt_update_once

function apt_install {
  apt_update_once
  echo -n "Installing ${@}: "
  if [ -n "${NO_INSTALL_SUDO}" ]; then
    apt-get install -yqq ${@} > /dev/null
  else
    sudo apt-get install -yqq ${@} > /dev/null
  fi
  success "Ok"
}
export -f apt_install

function located {
	if $( which ${1} > /dev/null 2>&1 ); then
		return 0
	else
    echo -n "${1}: "
    warning "Not found"
		return 1
	fi
}
export -f located

function get_area_pcase {
  local name=$1
  case "$name" in
		accountsdb)
      echo AccountsDB
		;;
    client)
      echo Client
		;;
    doc-site)
      echo Doc-Site
		;;
    jayne)
      echo Jayne
		;;
    native)
      echo Native
		;;
    native-deps)
      echo Native-Deps
		;;
    rediskeys)
      echo RedisKeys
		;;
    river)
      echo River
		;;
    serenity)
      echo Serenity
		;;
    tools)
      echo Tools
		;;
		*)
      error "Invalid area: ${1}"
			exit 1
		;;
	esac
}
export -f get_area_pcase

function maybe_run_script {	
	local dir=$1
  local name=$2
	local what=$3
  local quiet=$4
	if [ -f "${dir}/${name}.sh" ]; then
		if [ -z "${quiet}" ]; then
      echo -n "${what}: "
    fi
		set +e
		pushd ${dir} > /dev/null
    set -o pipefail
		if (( TRACE_ENABLED )); then
			bash "./${name}.sh" | tee "./${name}.log" 2>&1
		else
			bash "./${name}.sh" > "./${name}.log" 2>&1
		fi
		if [ $? -ne 0 ]  ; then
			error "Failed"
			error "====== ${name}.log ======"
			cat "./${name}.log"
			exit 1
		fi
		set -e
		set +o pipefail
		if [ -z "${quiet}" ]; then
      success "Ok"
    fi
		popd > /dev/null
    return 0
  else
    return 1
	fi
}
export -f maybe_run_script

function maybe_run_script_in_area {	
	local area=$1
	local script=$2
	local what=$3
  maybe_run_script "${RENDER_AREA_DIR}" "${area}.${script}" "${what}"
}
export -f maybe_run_script_in_area

function is_area_disabled {
  local area=$1
  if [ -z ${area} ]; then
    error "Area is missing"
    exit 1
  fi
  if [ -f "${ESTATE_BUILD_DIR}/in/${area}/disabled" ]; then
    return 0
  else
    return 1
  fi
}
export -f is_area_disabled