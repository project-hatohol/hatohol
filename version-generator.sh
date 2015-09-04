#!/bin/sh
version=15.07_dev1
if [ x$ADD_DATE_TO_VERSION = "x1" ]; then
  version=${version}_`eval date +%Y%m%d_%H%M%S`
fi
echo $version
