#!/usr/bin/env bash
#ESTATE_ACCOUNTS_DB_HOST="accountsdb-service"
#ESTATE_ACCOUNTS_DB_PORT="3306"
#ESTATE_ACCOUNTS_DB_USER="jayne"
#ESTATE_ACCOUNTS_DB_PASSWORD="5b7qokIQPxW2NA0LCBYp"
kubectl run -it --rm --image=mysql:5.7 --restart=Never mysql-client -- mysql -h accountsdb-service -p5b7qokIQPxW2NA0LCBYp -ujayne -Daccounts
