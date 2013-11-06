#!/bin/sh

export PYTHONPATH=../..:.
export DJANGO_SETTINGS_MODULE=testsettings 
../../manage.py syncdb
python -m unittest discover -p 'Test*.py'
