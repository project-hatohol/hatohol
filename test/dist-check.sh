#/bin/sh
export BASE_DIR="`dirname $0`"
top_dir="$BASE_DIR/.."
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
    *)
        :
        ;;
esac
$MAKE $MAKE_ARGS -C $top_dir/ hatoholdistcheck || exit 1
