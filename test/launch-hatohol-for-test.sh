#!/bin/sh

topdir=`echo $(cd $(dirname $0)/..; pwd)`
server_dir=$topdir/server
client_dir=$topdir/client
export LD_LIBRARY_PATH=$server_dir/mlpl/src/.libs:$server_dir/common/.libs:$server_dir/src/.libs

if test x"$NO_MAKE" != x"yes"; then
    if which gmake > /dev/null; then
        MAKE=${MAKE:-"gmake"}
    else
        MAKE=${MAKE:-"make"}
    fi
    case `uname` in
        Linux)
            MAKE_ARGS=${MAKE_ARGS:-"-j$(grep '^processor' /proc/cpuinfo | wc -l)"}
            ;;
        Darwin)
            MAKE_ARGS=${MAKE_ARGS:-"-j$(/usr/sbin/sysctl -n hw.ncpu)"}
            ;;
        *)
            :
            ;;
    esac
    $MAKE $MAKE_ARGS -C $topdir/ > /dev/null || exit 1
fi

mysql -uroot < $server_dir/data/create-db.sql
error_code=$?
if [ $error_code -ne 0 ]; then
  exit $error_code
fi

HATOHOL_DB_DIR=/tmp $server_dir/src/.libs/hatohol --pid-file /tmp/hatohol.pid --foreground --test-mode &
server_pid=$!
test ! -z $PIDS_FILE && echo $server_pid >> $PIDS_FILE

cd $client_dir
./manage.py syncdb
HATOHOL_DEBUG=1 ./manage.py runserver 0.0.0.0:8000 &
client_pid=$!
test ! -z $PIDS_FILE && echo $client_pid >> $PIDS_FILE

# wait for the boot completion
NUM_RETRY=30
RETRY_INTERVAL=0.5
n=0
while [ $n -le $NUM_RETRY ]
do
  test -d /proc/$server_pid || exit 1
  test -d /proc/$client_pid || exit 1

  http_code=`curl http://localhost:8000/tunnel/hello.html -o /dev/null -w '%{http_code}\n'` 
  if [ $http_code -eq 200 ]; then
    break
  fi
  
  sleep $RETRY_INTERVAL
  n=`expr $n + 1`
done
test $n -eq $NUM_RETRY && exit 1
exit 0
