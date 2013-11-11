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

DEFAULT_SERVER_ADDR = 'localhost'
DEFAULT_SERVER_PORT = 33194

SERVER_ADDR = DEFAULT_SERVER_ADDR
SERVER_PORT = DEFAULT_SERVER_PORT

SESSION_NAME_META = 'HTTP_X_HATOHOL_SESSION'
logger = logging.getLogger(__name__)

SERVER_ADDR_ENV_NAME = 'HATOHOL_SERVER_ADDR'
SERVER_PORT_ENV_NAME = 'HATOHOL_SERVER_PORT'

def _setup():
    # server
    global SERVER_ADDR
    server_addr = os.getenv(SERVER_ADDR_ENV_NAME)
    if server_addr:
        SERVER_ADDR = server_addr
        logger.info('Server addr: %s' % SERVER_ADDR)
    else:
        SERVER_ADDR = DEFAULT_SERVER_ADDR

    # port
    global SERVER_PORT
    server_port = os.getenv(SERVER_PORT_ENV_NAME)
    if server_port:
        SERVER_PORT = int(server_port)
        logger.info('Server port: %d' % SERVER_PORT)
    else:
        SERVER_PORT = DEFAULT_SERVER_PORT

def get_address():
    return SERVER_ADDR

def get_port():
    return SERVER_PORT

_setup()
