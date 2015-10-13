#!/bin/sh

date_version_package=0

while [ "$1" != "" ]; do
  if [ $1 == "-d" ]; then
    date_version_package=1
    echo Date Version Package: yes
  fi

  shift
done

if [ ! -d m4 ]; then
  mkdir m4
fi

# Deletetion m4 cache here to make sure to run
# m4_esyscmd() in configure.ac every time
rm -fr autom4te.cache
if [ $date_version_package -eq 1 ]; then
  export ADD_DATE_TO_VERSION=1
fi

autoreconf -i
