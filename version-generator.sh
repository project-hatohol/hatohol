#!/bin/sh
version=17.06
if [ x$ADD_DATE_TO_VERSION = "x1" ]; then
  version=${version}_`eval date +%Y%m%d_%H%M%S`
fi
echo -n $version
