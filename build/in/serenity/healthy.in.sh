#!/usr/bin/env bash
set -e
ss -ltpn | grep -E '{{ESTATE_SERENITY_GET_WORKER_PROCESS_ENDPOINT_PORT}}\s+0\.0\.0\.0:\*\s+users:\(\("{{ESTATE_SERENITY_DAEMON}}' > /dev/null