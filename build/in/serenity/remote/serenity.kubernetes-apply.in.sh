#!/usr/bin/env bash
set -ex
kubectl apply -k .
k_wait_rollout statefulsets/serenity-statefulset 600s