#!/bin/sh

sudo rabbitmqctl add_vhost test
sudo rabbitmqctl add_user test_user test_password
sudo rabbitmqctl set_permissions -p test test_user ".*" ".*" ".*"
