export PYTHONPATH=..
exitCode=0
./TestHatoholUtils.py || exitCode=1
./TestHatoholVoyager.py || exitCode=1
exit $exitCode
