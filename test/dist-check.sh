#/bin/sh
export BASE_DIR="`dirname $0`"
top_dir="$BASE_DIR/.."
if which gmake > /dev/null; then
    MAKE=${MAKE:-"gmake"}
else
    MAKE=${MAKE:-"make"}
fi
$MAKE $MAKE_ARGS -C $top_dir/ hatoholdistcheck || exit 1
