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
import argparse
from hatohol import HatoholActionCreator

DEFAULT_SERVER = "localhost"
DEFAULT_PORT = 33194

def print_usage():
  print "Usage: "
  print ""
  print "  $ hatohol-voyager [--server address[:port]] command [options]"
  print ""
  print "* commands"
  print ""
  print "  show-server [serverId]"
  print "  show-trigger [serverId [hostId [triggerId]]]"
  print "  show-event"
  print "  show-item"
  print "  show-host [sererId [hostId]]"
  print "  show-action"
  print "  add-action"
  print "    --type command|resident"
  print "    --command COMMAND_DEFINITION"
  print "    [--working-dir   DIR]"
  print "    [--timeout       TIMEOUT]"
  print "    [--server-id     SERVER_ID]"
  print "    [--host-id       HOST_ID]"
  print "    [--host-group-id HOST_GROUP_ID]"
  print "    [--trigger-id    TRIGGER_ID]"
  print "    [--status        TRIGGER_STATUS]"
  print "        TRIGGER_STATUS: ok or problem"
  print "    [--severity      COMPARATOR TRIGGER_SEVERITY]"
  print "        COMPARATOR       : -eq or -ge"
  print "        TRIGGER_SEVERITY : info, warn, or critical"
  print ""
  print "  del-action ACTION_ID"
  print ""

url_open_hook_func = None

def set_url_open_hook(func):
  global url_open_hook_func
  url_open_hook_func = func

def open_url(url):
  global url_open_hook_func
  if url_open_hook_func:
    url_open_hook_func(url)

  response = urllib2.urlopen(url)
  return response

def parse_server_arg(arg):
  words = arg.split(":")
  server = words[0]
  if (len(words) == 2):
    port = int(words[1])
  else:
    port = DEFAULT_PORT

  return "http://%s:%d" % (server, port)

def show_server(url, args):
  url += "/server"
  query = {}
  if args.serverId != None:
    query["serverId"] = args.serverId;
  if len(query) > 0:
    encoded_query = urllib.urlencode(query)
    url += "?" + encoded_query
  response = open_url(url)
  server_json = response.read()
  print server_json

def show_trigger(url, args):
  url += "/trigger"
  query = {}
  if args.serverId != None:
    query["serverId"] = args.serverId
  if args.hostId != None:
    query["hostId"] = args.hostId
  if args.triggerId != None:
    query["triggerId"] = args.triggerId
  if len(query) > 0:
    encoded_query = urllib.urlencode(query)
    url += "?" + encoded_query
  response = open_url(url)
  triggers_json = response.read()
  print triggers_json

def show_event(url, options):
  url += "/event"
  response = open_url(url)
  events_json = response.read()
  print events_json

def show_item(url, options):
  response = urllib2.urlopen(url + "/item")
  items_json = response.read()
  print items_json

def show_host(url, options):
  url += "/host"
  query = {}
  if len(options) >= 1:
    query["serverId"] = options[0]
  if len(options) >= 2:
    query["hostId"] = options[1]
  if len(query) > 0:
    encoded_query = urllib.urlencode(query)
    url += "?" + encoded_query
  response = urllib2.urlopen(url)
  items_json = response.read()
  print items_json

def show_action(url, options):
  response = urllib2.urlopen(url + "/action")
  actions_json = response.read()
  print actions_json

def add_action(url, options):
  action_creator = HatoholActionCreator.HatoholActionCreator(url)
  action_creator.add(options)

def del_action(url, options):
  if len(options) == 0:
    print "Need actoin ID. "
    sys.exit(-1)
  action_id = options[0]
  req = urllib2.Request(url + "/action/" + action_id)
  req.get_method = lambda: 'DELETE'
  response = urllib2.urlopen(req)
  result_json = response.read()
  print result_json

command_map = {
  "show-server":show_server,
  "show-event":show_event,
  "show-trigger":show_trigger,
  "show-item":show_item,
  "show-host":show_host,
  "show-action":show_action,
  "add-action":add_action,
  "del-action":del_action,
}

def main(arg_list=None):
  parser = argparse.ArgumentParser(description="Hatohol Voyager")
  parser.add_argument("--server", type=parse_server_arg, dest="server_url",
                      metavar="SERVER[:PORT]",
                      default="%s:%d" % (DEFAULT_SERVER, DEFAULT_PORT))
  subparsers = parser.add_subparsers(help="Sub commands", dest="sub_command")

  # server
  sub_server = subparsers.add_parser("show-server")
  sub_server.add_argument("serverId", type=int, nargs="?",
                          help="get a server only with serverId")
  # trigger
  sub_trigger = subparsers.add_parser("show-trigger")
  sub_trigger.add_argument("serverId", type=int, nargs="?")
  sub_trigger.add_argument("hostId", type=int, nargs="?")
  sub_trigger.add_argument("triggerId", type=int, nargs="?")

  # event
  sub_trigger = subparsers.add_parser("show-event")

  args = parser.parse_args(arg_list)
  command_map[args.sub_command](args.server_url, args)
