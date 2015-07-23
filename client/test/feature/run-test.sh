#!/bin/sh
export BASE_DIR="`dirname $0`"
top_dir="$BASE_DIR/../.."
top_dir="`cd $top_dir; pwd`"

LANG=ja_JP.utf8 LC_ALL=ja_JP.UTF-8 $(npm bin)/casperjs test $top_dir/test/feature/*_test.js
