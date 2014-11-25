#!/bin/sh

#
# Example
# $ ./run-tesh.sh TestUserConfig.TestUserConfig.test_get_integer
#

cd "$(dirname "$0")"

testcase=""
if [ $# -ge 1 ]; then
  testcase=$1
fi

export PYTHONPATH=../..:.
export DJANGO_SETTINGS_MODULE=testsettings 
export HATOHOL_SERVER_PORT=54321
../../manage.py syncdb

if [ -z $testcase ]; then
  python -m unittest discover -p 'Test*.py'
else
  python -m unittest $testcase
fi

