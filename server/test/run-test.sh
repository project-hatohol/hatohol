unset LANG

export BASE_DIR="`dirname $0`"
top_dir="$BASE_DIR/.."
top_dir="`cd $top_dir; pwd`"

if test x"$NO_MAKE" != x"yes"; then
    if which gmake > /dev/null; then
	MAKE=${MAKE:-"gmake"}
    else
	MAKE=${MAKE:-"make"}
    fi
    $MAKE -C $top_dir/ > /dev/null || exit 1
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
