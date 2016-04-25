#!/bin/sh
version=16.04beta1
if [ x$ADD_DATE_TO_VERSION = "x1" ]; then
  version=${version}_`eval date +%Y%m%d_%H%M%S`
fi
echo -n $version
