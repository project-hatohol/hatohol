#!/bin/sh

topdir=`echo $(cd $(dirname $0)/..; pwd)`
server_dir=$topdir/server
client_dir=$topdir/client
export LD_LIBRARY_PATH=$server_dir/mlpl/src/.libs:$server_dir/src/.libs

expect -c "
set timeout 30
spawn $server_dir/tools/hatohol-config-db-creator $topdir/test/hatohol-config.dat
expect \"Please input the root password for the above server.\\\"(If you've not set the root password, just push enter)\"
send \"\n\"
expect \"The database: hatohol will be dropped. Are you OK? (yes/no)\"
send \"yes\n\"
interact
"
error_code=$?
if [ $error_code -ne 0 ]; then
  exit $exit_code
fi

HATOHOL_DB_DIR=/tmp $server_dir/src/.libs/hatohol --pid-file-path /tmp/hatohol.pid --foreground --test-mode &
server_pid=$!

cd $client_dir
HATOHOL_DEBUG=1 ./manage.py runserver 0.0.0.0:8000 &
client_pid=$!

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
exit 0
