#!/usr/bin/env python
# coding: UTF-8
#
#  Copyright (C) 2015 Project Hatohol
#
#  This file is part of Hatohol.
#
#  Hatohol is free software: you can redistribute it and/or modify
#  it under the terms of the GNU Lesser General Public License, version 3
#  as published by the Free Software Foundation.
#
#  Hatohol is distributed in the hope that it will be useful,
#  but WITHOUT ANY WARRANTY; without even the implied warranty of
#  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
#  GNU Lesser General Public License for more details.
#
#  You should have received a copy of the GNU Lesser General Public
#  License along with Hatohol. If not, see
#  <http://www.gnu.org/licenses/>.

"""
This module contains basic functions and class for HAP (Hatohol Arm Plugins)
and is supposed to be used both from haplib and transporter.
transporter and the sub class shall not import haplib. So the functions and
classes used in them have to be in this module.
"""

import logging
import sys
import traceback

def handle_exception(raises=(SystemExit,)):
    """
    Logging exception information including back trace and return
    some information. This method is supposed to be used in 'except:' block.
    Note that if the exception class is Signal, this method doesn't log it.

    @raises
    A sequence of exceptionclass names. If the handling exception is one of
    it, this method just raises it again.

    @return
    A sequence of exception class and the instance of the handling exception.
    """
    (exctype, value, tb) = sys.exc_info()
    if exctype in raises:
        raise
    if exctype is not Signal:
        logging.error("Unexpected error: %s, %s, %s" % \
                      (exctype, value, traceback.format_tb(tb)))
    return exctype, value


class Signal:
    """
    This class is supposed to raise as an exception in order to
    propagate some events and jump over stack frames.
    """

    def __init__(self, restart=False, critical=False):
        self.restart = restart
        self.critical = critical
