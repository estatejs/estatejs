#!/usr/bin/env bash
echo "Disabled"

# set -e
# cd "$(dirname "$0")" || exit 1
# source "../build/util/.functions.sh"
# cd ../build

# ensure_env

# FROM_NAME=$(basename "${1}")
# shift
# BUILD_TARGET="${1}"
# shift
# BUILD_TYPE="${1}"
# shift

# BRANCH=$(git rev-parse --abbrev-ref HEAD)
# if [ "${BRANCH}" != "${BUILD_TARGET}" ]; then
#   error "Must switch to the ${BUILD_TARGET} branch."
#   exit 1
# fi

# IS_LOCAL=0
# IS_PRODUCTION=0
# if [ "${BUILD_TARGET}" == "local" ]; then
#   IS_LOCAL=1
# elif [ "${BUILD_TARGET}" == "production" ]; then
#   IS_PRODUCTION=1
# fi

# PHASE="all"
# if [ -n "${1}" ]; then
#   PHASE="${1}"
#   shift
# fi

# confirm() {
#   local _prompt _default _response

#   if [ "$1" ]; then _prompt="$1"; else _prompt="Are you sure"; fi
#   _prompt="$_prompt [y/n] ?"

#   # Loop forever until the user enters a valid response (Y/N or Yes/No).
#   while true; do
#     read -r -p "$_prompt " _response
#     case "$_response" in
#     [Yy][Ee][Ss] | [Yy]) # Yes or Y (case-insensitive).
#       return 0
#       ;;
#     [Nn][Oo] | [Nn]) # No or N.
#       return 1
#       ;;
#     *) # Anything else (including a blank) is invalid.
#       ;;
#     esac
#   done
# }

# function stop_local {
#   local area=$1
#   local_deploy_stop "${area}" "${ESTATE_PLATFORM_DIR}/build/out/${BUILD_TARGET}.${BUILD_TYPE}/${area}/${area}.pid"
# }

# function update_log_prefix {
#   export LOG_PREFIX="\e[1;32m(${1})\e[0m\e[1;33m[${FROM_NAME}]\e[0m "
# }

# function kill_ {
#   update_log_prefix "kill"

#   if ((IS_LOCAL)); then
#     info_n "${LOG_PREFIX}Shutting down local platform before re-rendering: "
#     pkill -f ".yarn/bin/serve" || true
#     pkill -f Estate.Jayne || true
#     pkill -f serenityd || true
#     pkill -f riverd || true
#     success "Ok"
#   else
#     warning "${LOG_PREFIX}Not killing non-local platform."
#   fi
# }

# function render {
#   update_log_prefix "render"

#   # Always start from scratch
#   rm -rf out/ >/dev/null

#   info "${LOG_PREFIX}Rendering.."
#   ./render.sh "${BUILD_TARGET}" "${BUILD_TYPE}" all
# }

# function build_platform {
#   update_log_prefix "build-platform"

#   info "${LOG_PREFIX}Building the platform..."
#   ./build.sh "${BUILD_TARGET}" "${BUILD_TYPE}" serenity river jayne
# }

# function deploy_platform {
#   update_log_prefix "deploy-platform"

#   if ((IS_LOCAL)); then
#     info "${LOG_PREFIX}Deploying the platform locally.."
#   else
#     info "${LOG_PREFIX}Deploying the platform to ${BUILD_TARGET}.."
#   fi
#   ./deploy.sh "${BUILD_TARGET}" "${BUILD_TYPE}" serenity river jayne accountsdb rediskeys
# }

# function build_clients {
#   update_log_prefix "build-clients"

#   info "${LOG_PREFIX}Building client, tools, and site.."
#   cd "${ESTATE_PLATFORM_DIR}/build"
#   ./build.sh "${BUILD_TARGET}" "${BUILD_TYPE}" client tools site
# }

# function deploy_site {
#   update_log_prefix "deploy-site"

#   info "${LOG_PREFIX}Deploying site.."
#   ./deploy.sh "${BUILD_TARGET}" "${BUILD_TYPE}" site
# }

# function test_setup {
#   update_log_prefix "test-setup"

#   info "${LOG_PREFIX}Running test setup.."
#   ./test-setup.sh "${BUILD_TARGET}" "${BUILD_TYPE}"
# }

# function test {
#   update_log_prefix "test"

#   info "${LOG_PREFIX}Running test.."
#   ./test.sh "${BUILD_TARGET}" "${BUILD_TYPE}"
# }

# function test_cleanup {
#   update_log_prefix "test-cleanup"

#   info "${LOG_PREFIX}Cleaing up test run.."
#   ./test-cleanup.sh "${BUILD_TARGET}" "${BUILD_TYPE}"
# }

# function publish {
#   update_log_prefix "publish"

#   # TODO: create test client and tools NPM package so we can verify that stuff works too
#   if ((IS_PRODUCTION)); then
#     info "${LOG_PREFIX}Performing client publish dry-run.."
#     ./publish.sh "${BUILD_TARGET}" "${BUILD_TYPE}" client
#     if confirm "Publish client"; then
#       ./publish.sh "${BUILD_TARGET}" "${BUILD_TYPE}" client --accept
#     fi

#     info "${LOG_PREFIX}Performing tools publish dry-run.."
#     ./publish.sh "${BUILD_TARGET}" "${BUILD_TYPE}" tools
#     if confirm "Publish tools"; then
#       ./publish.sh "${BUILD_TARGET}" "${BUILD_TYPE}" tools --accept
#     fi
#   else
#     info "${LOG_PREFIX}Not publishing non-production client and tools."
#   fi
# }

# function shutdown {
#   update_log_prefix "shutdown"

#   if ((IS_LOCAL)); then
#     if confirm "Shutdown local platform"; then
#       info_n "${LOG_PREFIX}Shutting down local platform: "
#       stop_local site
#       stop_local jayne
#       stop_local river
#       stop_local serenity
#       success "Ok"
#     fi
#   else
#     warning "${LOG_PREFIX}Not shutting down non-local deployment."
#   fi
# }

# function backup {
#   update_log_prefix "backup"

#   if ((IS_PRODUCTION)); then
#     if confirm "Backup production deployment"; then
#       info_n "${LOG_PREFIX}Backing up successful production deployment: "
#       tar zcvf "${ESTATE_PLATFORM_DIR}/../s-deployments/${BUILD_TARGET}.${BUILD_TYPE}.tgz" "${ESTATE_PLATFORM_DIR}/build/out/${BUILD_TARGET}.${BUILD_TYPE}" >/dev/null 2>&1
#       success "Ok"
#     fi
#   else
#     warning "${LOG_PREFIX}Not backing up non-production deployment."
#   fi
# }

# case "${PHASE}" in
# "all") ;&
# "kill")
#   kill_
#   ;&
# "render")
#   render
#   ;&
# "build-platform")
#   build_platform
#   ;&
# "deploy-platform")
#   deploy_platform
#   ;&
# "build-clients")
#   build_clients
#   ;&
# "deploy-site")
#   deploy_site
#   ;&
# "test-setup")
#   test_setup
#   ;&
# "test")
#   test
#   ;&
# "test-cleanup")
#   test_cleanup
#   ;&
# "publish")
#   publish
#   ;&
# "shutdown")
#   shutdown
#   ;&
# "backup")
#   backup
#   ;;
# esac
