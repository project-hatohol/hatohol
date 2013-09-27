#!/usr/bin/env python
"""
  Copyright (C) 2013 Project Hatohol

  This file is part of Hatohol.

  Hatohol is free software: you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation, either version 2 of the License, or
  (at your option) any later version.

  Hatohol is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with Hatohol. If not, see <http://www.gnu.org/licenses/>.
"""

import sys
import urllib
import urllib2
import hatohol

class HatoholActionCreator:
  def __init__(self, url):
    self._url = url

  def add(self, options):
    type = self._get_one_arg(options, "--type")
    type_code = None
    if type == "command":
      type_code = hatohol.ACTION_COMMAND
    elif type == "resident":
      type_code = hatohol.ACTION_RESIDENT
    else:
      print "Type must be 'command' or 'resident'."
      sys.exit(-1)

    command = self._get_one_arg(options, "--command")
    working_dir = self._get_one_arg(options, "--working-dir", True)
    timeout = self._get_one_arg(options, "--timeout", True)
    server_id = self._get_one_arg(options, "--server-id", True)
    host_id = self._get_one_arg(options, "--host-id", True)
    host_group_id = self._get_one_arg(options, "--host-group-id", True)
    trigger_id = self._get_one_arg(options, "--trigger-id", True)

    status = self._get_one_arg(options, "--status", True)
    status_code = None
    if status == "ok":
      status_code = hatohol.TRIGGER_STATUS_OK
    elif status == "problem":
      status_code = hatohol.TRIGGER_STATUS_PROBLEM
    else:
      print "Status must be 'ok' or 'problem'."
      sys.exit(-1)

    (severity_cmp, severity) = self._get_arg(options, "--severity", True, 2)
    severity_cmp_code = None
    severity_code = None
    if severity:
      if severity == "info":
        severity_code = hatohol.TRIGGER_SEVERITY_INFO
      elif severity == "warn":
        severity_code = hatohol.TRIGGER_SEVERITY_WARN
      elif severity == "critical":
        severity_code = hatohol.TRIGGER_SEVERITY_CRITICAL
      else:
        print "Severity must be 'info', 'warn', or 'critical'."
        sys.exit(-1)

      if severity_cmp == "-eq":
        severity_cmp_code = hatohol.CMP_EQ
      elif severity_cmp == "-ge":
        severity_cmp_code = hatohol.CMP_EQ_GT
      else:
        print "Severity comparator must be '-eq' or '-ge'."
        sys.exit(-1)

    print "Type       : " + type
    print "Commad     : " + command
    if working_dir:
      print "Working dir: " + working_dir
    if timeout:
      print "Timeout    : " + timeout
    if server_id:
      print "Server ID  : " + server_id
    if host_id:
      print "Host ID    : " + host_id
    if host_group_id:
      print "Host Grp ID: " + host_group_id
    if trigger_id:
      print "Trigger ID : " + trigger_id
    if status:
      print "Trig. Stat.: " + status
    if severity:
      print "Seveirty   : " + severity_cmp + " " + severity

    self._send(type_code, command, working_dir, timeout, server_id, host_id, host_group_id, trigger_id, status_code, severity_cmp_code, severity_code)

  def _send(self, type_code, command, working_dir, timeout, server_id, host_id, host_group_id, trigger_id, status_code, severity_cmp_code, severity_code):
    query = {}
    query["type"] = str(type_code)
    query["command"] = command
    if working_dir:
      query["workingDirectory"] = working_dir
    if timeout:
      query["timeout"] = str(timeout)
    if server_id:
      query["serverId"] = str(server_id)
    if host_id:
      query["hostId"] = str(host_id)
    if host_group_id:
      query["hostGroupId"] = str(host_group_id)
    if trigger_id:
      query["triggerId"] = str(trigger_id)
    if status_code:
      query["triggerStatus"] = str(status_code)
    if severity_code:
      query["triggerSeverity"] = str(severity_code)
      query["triggerSeverityCompType"] = str(severity_cmp_code)

    encoded_query = urllib.urlencode(query)
    url = self._url + "/action"
    print "URL: " + url
    response = urllib2.urlopen(url, encoded_query)
    print response.read()

  def _get_arg(self, options, key, allow_none = False, num_arg=1):
    if key not in options:
      if allow_none:
        args = []
        for i in range(num_arg):
          args.append(None)
        return args
      print key + " is not defined."
      sys.exit(-1)
    idx = options.index(key)
    if idx >= len(options) - num_arg:
      print key + ": must have the argument."
      sys.exit(-1)
    args = options[idx+1:idx+num_arg+1]
    del options[idx:idx+num_arg+1]
    return args

  def _get_one_arg(self, options, key, allow_none = False):
    return self._get_arg(options, key, allow_none)[0]

