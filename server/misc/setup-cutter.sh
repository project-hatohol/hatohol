#!/bin/sh

NUM_RETRY=10
n=0
exit_code=0
while [ $n -lt $NUM_RETRY ]
do
  # The following setup.sh hasn't worked since about 11/05 evening (JST).
  # Now I'm asking this matter to the developper at
  # https://github.com/clear-code/cutter/commit/bf343f9538d46ea2ac9d8ca673b2e60e97bf8eab#commitcomment-4520815
  # Here, we temporarily use an easy workaround that gives the previous URLs to apt.
  #curl https://raw.github.com/clear-code/cutter/master/data/travis/setup.sh | sh
  curl https://raw.github.com/clear-code/cutter/master/data/travis/setup.sh | sed s,net/cutter,net/project/cutter, | sh
  exit_code=$?
  if [ $exit_code -eq 0 ]; then
    break
  fi
  n=`expr $n + 1`
done
exit $exit_code
