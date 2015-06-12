#!/bin/sh

test_dir=$(cd $(dirname $0) && pwd)
export PYTHONPATH=$test_dir/..
python -m unittest discover -s $test_dir -p 'Test*.py' $1 $2 $3 $4 $5 $6 $7 $8 $9
