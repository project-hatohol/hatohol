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
import argparse

class SeverityParseAction(argparse.Action):
  def __call__(self, parser, namespace, values, option_string=None):
    if len(values) != 2:
       raise argparse.ArgumentTypeError("--severity expects 2 arguments.")

    severity_cmp_dict = {"eq":hatohol.CMP_EQ, "ge":hatohol.CMP_EQ_GT}
    severity_dict = {"info":hatohol.TRIGGER_SEVERITY_INFO,
                     "warn":hatohol.TRIGGER_SEVERITY_WARNING,
                     "error":hatohol.TRIGGER_SEVERITY_ERROR,
                     "critical":hatohol.TRIGGER_SEVERITY_CRITICAL,
                     "emergency":hatohol.TRIGGER_SEVERITY_EMERGENCY}
    comparator = values[0]
    if comparator not in severity_cmp_dict:
       raise argparse.ArgumentTypeError("comparator must be: eq or ge (%s)" %
                                        comparator)
    severity = values[1]
    if severity not in severity_dict:
       raise argparse.ArgumentTypeError("severity must be: info, warn, error, critical, or emergency (%s)" % severity)

    setattr(namespace, "severity_cmp", comparator)
    setattr(namespace, "severity", severity)

    setattr(namespace, "severity_cmp_code", severity_cmp_dict[comparator])
    setattr(namespace, "severity_code", severity_dict[severity])



class HatoholActionCreator:
  def __init__(self, url):
    self._url = url + "/action"
    self._encoded_query = None

  @classmethod
  def setup_arguments(cls, parser):
    parser.add_argument("--type", choices=["command", "resident"],
                        required=True)
    parser.add_argument("--command", required=True)
    parser.add_argument("--working-dir")
    parser.add_argument("--timeout")
    parser.add_argument("--server-id", type=int)
    parser.add_argument("--host-id", type=int)
    parser.add_argument("--host-group-id", type=int)
    parser.add_argument("--trigger-id", type=int)
    parser.add_argument("--status", choices=["ok", "problem"])
    parser.add_argument("--severity", nargs=2,
                        metavar=("SEVERITY_CMP", "SEVERITY"),
                        action=SeverityParseAction,
                        help="{eq,ge} {info,warn,error,critical,emergency}")

  def get_url(self):
    return self._url

  def get_encoded_query(self):
    return self._encoded_query

  def add(self, args):
    type_code = None
    if args.type == "command":
      type_code = hatohol.ACTION_COMMAND
    elif args.type == "resident":
      type_code = hatohol.ACTION_RESIDENT
    else:
      print "Type must be 'command' or 'resident'."
      sys.exit(-1)

    server_id = args.server_id
    trigger_id = args.trigger_id

    status = args.status
    status_code = None
    if status == "ok":
      status_code = hatohol.TRIGGER_STATUS_OK
    elif status == "problem":
      status_code = hatohol.TRIGGER_STATUS_PROBLEM

    print "Type       : " + args.type
    print "Commad     : " + args.command
    if args.working_dir is not None:
      print "Working dir: " + args.working_dir
    if args.timeout is not None:
      print "Timeout    : " + str(args.timeout)
    if args.server_id is not None:
      print "Server ID  : " + str(args.server_id)
    if args.host_id is not None:
      print "Host ID    : " + str(args.host_id)
    if args.host_group_id is not None:
      print "Host Grp ID: " + str(args.host_group_id)
    if args.trigger_id is not None:
      print "Trigger ID : " + str(args.trigger_id)
    if args.status is not None:
      print "Trig. Stat.: " + args.status
    if args.severity is not None:
      print "Seveirty   : " + args.severity_cmp + " " + args.severity

    query = {}
    query["type"] = str(type_code)
    query["command"] = args.command
    if args.working_dir is not None:
      query["workingDirectory"] = args.working_dir
    if args.timeout is not None:
      query["timeout"] = str(args.timeout)
    if args.server_id is not None:
      query["serverId"] = str(args.server_id)
    if args.host_id is not None:
      query["hostId"] = str(args.host_id)
    if args.host_group_id is not None:
      query["hostGroupId"] = str(args.host_group_id)
    if args.trigger_id is not None:
      query["triggerId"] = str(args.trigger_id)
    if status_code is not None:
      query["triggerStatus"] = str(status_code)
    if args.severity_code is not None:
      query["triggerSeverity"] = str(args.severity_code)
      query["triggerSeverityCompType"] = str(args.severity_cmp_code)

    self._encoded_query = urllib.urlencode(query)
    print "URL: " + self._url
