#!/bin/sh

#
# Example
# $ ./run-tesh.sh TestUserConfig.TestUserConfig.test_get_integer
#

export BASE_DIR="`dirname $0`"
top_dir="$BASE_DIR/../../.."
top_dir="`cd $top_dir; pwd`"
client_dir="$top_dir/client"

if test x"$NO_MAKE" != x"yes"; then
    if which gmake > /dev/null; then
        MAKE=${MAKE:-"gmake"}
    else
        MAKE=${MAKE:-"make"}
    fi
    MAKE_ARGS=
    $MAKE $MAKE_ARGS -C $client_dir/ hatohol/hatohol_def.py || exit 1
fi

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

