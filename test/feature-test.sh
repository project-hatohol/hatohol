#/bin/sh
set -e

topdir=`echo $(cd $(dirname $0)/..; pwd)`

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
        *)
            :
            ;;
    esac
    $MAKE $MAKE_ARGS -C $topdir/ || exit 1
fi

sudo make install

sudo rabbitmqctl add_user test_user test_password
sudo rabbitmqctl add_vhost test
sudo rabbitmqctl set_permissions -p test test_user ".*" ".*" ".*"

./test/launch-hatohol-for-test.sh
LD_LIBRARY_PATH=/usr/local/lib:$LD_LIBRARY_PATH hatohol-db-initiator
LANG=ja_JP.utf8 LC_ALL=ja_JP.UTF-8 ./client/test/feature/run-test.sh
