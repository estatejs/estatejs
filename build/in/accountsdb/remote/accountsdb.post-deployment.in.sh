#!/usr/bin/env bash
set -ex
echo "Getting accountsdb pod name"
POD_NAME=$(kubectl get pods -l='app=accountsdb' -o='NAME')
if [ -z "${POD_NAME}" ]; then
  echo "Failed to get pod name"
  exit 1;
fi

echo "Applying accountsdb schema"
kubectl exec -i "${POD_NAME}" -- mysql -p"{{ESTATE_ACCOUNTS_DB_PASSWORD}}" -u"{{ESTATE_ACCOUNTS_DB_USER}}" < ./accountsdb.sql