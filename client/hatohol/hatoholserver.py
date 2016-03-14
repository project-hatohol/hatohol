# Copyright (C) 2013,2016 Project Hatohol
#
# This file is part of Hatohol.
#
# Hatohol is free software: you can redistribute it and/or modify
# it under the terms of the GNU Lesser General Public License, version 3
# as published by the Free Software Foundation.
#
# Hatohol is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
# GNU Lesser General Public License for more details.
#
# You should have received a copy of the GNU Lesser General Public
# License along with Hatohol. If not, see
# <http://www.gnu.org/licenses/>.

import os
import logging
from django.conf import settings

SESSION_NAME_META = 'HTTP_X_HATOHOL_SESSION'

# As the following source code, priority of the way to select
# the Hatohol server is the following.
#
# 1. Environment variable
# 2. settings (webui.conf)
# 3. This module's default
#
DEFAULT_SERVER_ADDR = 'localhost'
DEFAULT_SERVER_PORT = 33194

SERVER_ADDR_ENV_NAME = 'HATOHOL_SERVER_ADDR'
SERVER_PORT_ENV_NAME = 'HATOHOL_SERVER_PORT'

SERVER_ADDR = None
SERVER_PORT = None

# 1. Env. var.
if SERVER_ADDR is None:
    addr = os.getenv(SERVER_ADDR_ENV_NAME)
    if addr:
        SERVER_ADDR = addr

if SERVER_PORT is None:
    port = os.getenv(SERVER_PORT_ENV_NAME)
    if port:
        SERVER_PORT = int(port)

# 2. settings.HATOHOL_SERVER_ADDR and PORT are read from 'webui.conf'
if SERVER_ADDR is None:
    if hasattr(settings, 'HATOHOL_SERVER_ADDR'):
        SERVER_ADDR = settings.HATOHOL_SERVER_ADDR

if SERVER_PORT is None:
    if hasattr(settings, 'HATOHOL_SERVER_PORT'):
        SERVER_PORT = settings.HATOHOL_SERVER_PORT

# 3. Default
if SERVER_ADDR is None:
    SERVER_ADDR = DEFAULT_SERVER_ADDR
if SERVER_PORT is None:
    SERVER_PORT = DEFAULT_SERVER_PORT

logger = logging.getLogger(__name__)
logger.info('Hatohol server addr: %s, port: %d' % (SERVER_ADDR, SERVER_PORT))


def get_address():
    return SERVER_ADDR


def get_port():
    return SERVER_PORT
