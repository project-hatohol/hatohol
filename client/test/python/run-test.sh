#!/bin/sh

export PYTHONPATH=../..
export DJANGO_SETTINGS_MODULE=hatohol.settings 
python -m unittest discover -p 'Test*.py'
