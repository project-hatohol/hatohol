#!/bin/sh

unset LANG

export PATH=../src/.libs:../mlpl/src/.libs:$PATH
export LD_LIBRARY_PATH=../src/.libs:../mlpl/src/.libs:$LD_LIBRARY_PATH

export HATOHOL_ACTION_LD_LIBRARY_PATH=../src/.libs:../mlpl/src/.libs

export BASE_DIR="`dirname $0`"
top_dir="$BASE_DIR/../.."
top_dir="`cd $top_dir; pwd`"

if test x"$NO_MAKE" != x"yes"; then
    if which gmake > /dev/null; then
        MAKE=${MAKE:-"gmake"}
    else
        MAKE=${MAKE:-"make"}
    fi
    MAKE_ARGS=
    case `uname` in
        Linux)
            MAKE_ARGS="-j$(grep '^processor' /proc/cpuinfo | wc -l)"
            ;;
        Darwin)
            MAKE_ARGS="-j$(/usr/sbin/sysctl -n hw.ncpu)"
            ;;
        *)
            :
            ;;
    esac
    $MAKE $MAKE_ARGS -C $top_dir/ > /dev/null || exit 1
fi

tmpfs_dir=/dev/shm
if test -z "$HATOHOL_DB_DIR" -a -d "$tmpfs_dir"; then
    HATOHOL_DB_DIR="$tmpfs_dir/hatohol"
    export HATOHOL_DB_DIR
    mkdir -p "$HATOHOL_DB_DIR"
fi

if test -z "$CUTTER"; then
    CUTTER="`make -s -C $BASE_DIR echo-cutter`"
fi

CUTTER_ARGS="--notify=no"
CUTTER_WRAPPER=
if test x"$CUTTER_DEBUG" = x"yes"; then
    CUTTER_WRAPPER="$top_dir/libtool --mode=execute gdb --args"
    CUTTER_ARGS="--keep-opening-modules"
elif test x"$CUTTER_CHECK_LEAK" = x"yes"; then
    CUTTER_WRAPPER="$top_dir/libtool --mode=execute valgrind "
    CUTTER_WRAPPER="$CUTTER_WRAPPER --leak-check=full --show-reachable=yes -v"
    CUTTER_ARGS="--keep-opening-modules"
fi

export CUTTER

CUTTER_ARGS="$CUTTER_ARGS -s $BASE_DIR --exclude-directory fixtures"
if echo "$@" | grep -- --mode=analyze > /dev/null; then
    :
fi
if test x"$USE_GTK" = x"yes"; then
    CUTTER_ARGS="-u gtk $CUTTER_ARGS"
fi

CUTTER_ARGS="$CUTTER_ARGS -v v"
$CUTTER_WRAPPER $CUTTER $CUTTER_ARGS "$@" $BASE_DIR
