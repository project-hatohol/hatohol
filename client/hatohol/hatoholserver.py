# Copyright (C) 2013 Project Hatohol
#
# This file is part of Hatohol.
#
# Hatohol is free software: you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation, either version 2 of the License, or
# (at your option) any later version.
#
# Hatohol is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with Hatohol. If not, see <http://www.gnu.org/licenses/>.

import os
import logging

SERVER_ADDR = 'localhost'
SERVER_PORT = 33194
SESSION_NAME_META = 'HTTP_X_HATOHOL_SESSION'

def _setup():
    global SERVER_PORT
    server_port = os.getenv('HATOHOL_SERVER_PORT')
    if server_port:
        SERVER_PORT = int(server_port)
        logger = logging.getLogger(__name__)
        logger.info('Server port: %d' % SERVER_PORT)

def get_address():
    return SERVER_ADDR

def get_port():
    return SERVER_PORT

_setup()
