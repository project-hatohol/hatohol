#!/bin/sh
mkdir -p /etc/rabbitmq/rabbitmq.conf.d
echo RABBITMQ_NODE_PORT=5673 > /etc/rabbitmq/rabbitmq.conf.d/port
