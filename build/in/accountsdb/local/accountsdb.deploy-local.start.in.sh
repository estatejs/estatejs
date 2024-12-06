#!/usr/bin/env bash
set -ex

pushd {{LOCAL_DEPLOY_RUN_DIR}}

echo "Starting accountsdb..."
docker-compose start accountsdb

echo "Waiting for MySQL to come online..."
until docker-compose exec -T accountsdb mysql -p"{{ESTATE_ACCOUNTS_DB_PASSWORD}}" -u"{{ESTATE_ACCOUNTS_DB_USER}}" -e "select 1"
do
  sleep 1
done

echo "Applying accountsdb schema..."
docker-compose exec -T accountsdb mysql -p"{{ESTATE_ACCOUNTS_DB_PASSWORD}}" -u"{{ESTATE_ACCOUNTS_DB_USER}}" < {{RENDER_AREA_DIR}}/accountsdb.sql