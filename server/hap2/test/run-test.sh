#!/bin/sh

test_dir=$(cd $(dirname $0) && pwd)
export PYTHONPATH=$test_dir/..
python -m unittest discover -s $test_dir -p 'Test*.py' "$@"
