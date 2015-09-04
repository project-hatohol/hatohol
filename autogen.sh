#!/bin/sh

# The date and time can be added to the version by
# setting the environment variable like
#
#   $ rm -fr autom4te.cache
#   $ ADD_DATE_TO_VERSION=1 ./autogen.sh

if [ ! -d m4 ]; then
  mkdir m4
fi

autoreconf -i
