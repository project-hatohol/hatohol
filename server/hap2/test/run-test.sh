#!/bin/sh

test_dir=$(cd $(dirname $0) && pwd)
export PYTHONPATH=$test_dir/..

# Workaround: probably due to amqp-tools bug.
if [ `cat /etc/issue | grep "^Ubuntu 14.04" | wc -l` = "1" ]; then
  export RABBITMQ_CONNECTOR_TEST_WORKAROUND=1
fi
python -m unittest discover -s $test_dir -p 'Test*.py' "$@"
