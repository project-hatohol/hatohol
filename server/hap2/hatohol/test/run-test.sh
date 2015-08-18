#!/bin/sh

test_dir=$(cd $(dirname $0) && pwd)
export PYTHONPATH=$test_dir/../:$test_dir/../../

# Workaround: probably due to amqp-tools bug.
WORK_AROUND_VERSION="0.4.1"
CURRENT_VERSION=`pkg-config --modversion librabbitmq`
awk 'BEGIN{if ('$WORK_AROUND_VERSION'<='$CURRENT_VERSION') exit 1}'
if [ $? -eq 1 ]; then
  export RABBITMQ_CONNECTOR_TEST_WORKAROUND=1
fi

RABBITMQ_PORT_CONF=/etc/rabbitmq/rabbitmq.conf.d/port
if [ -f $RABBITMQ_PORT_CONF ]; then
  . $RABBITMQ_PORT_CONF
  export RABBITMQ_NODE_PORT
fi
python -m unittest discover -s $test_dir -p 'Test*.py' "$@"
