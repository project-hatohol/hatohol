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
import hatohol
from hatohol.ActionCreator import ActionCreator

DEFAULT_SERVER = "localhost"
DEFAULT_PORT = 33194

class UserCreator:
  def __init__(self, url):
    self._url = url + "/user"
    self._encoded_query = None

  @classmethod
  def setup_arguments(cls, parser):
    parser.add_argument("user")
    parser.add_argument("password")
    parser.add_argument("flags")

  def get_url(self):
    return self._url

  def get_encoded_query(self):
    return self._encoded_query

  def add(self, args):
    query = {}
    query["user"] = args.user;
    query["password"] = args.password;
    query["flags"] = args.flags;
    self._encoded_query = urllib.urlencode(query)

def open_url_and_show_response(cmd_ctx):
  if "encoded_query" in cmd_ctx:
    response = urllib2.urlopen(cmd_ctx["url"], cmd_ctx["encoded_query"])
  else:
    response = urllib2.urlopen(cmd_ctx["url"])
  print response.read()

def parse_server_arg(arg):
  words = arg.split(":")
  server = words[0]
  if (len(words) == 2):
    port = int(words[1])
  else:
    port = DEFAULT_PORT

  return "http://%s:%d" % (server, port)

def do_test(url, args):
  url += "/test"
  return {"url":url, "postproc":open_url_and_show_response}

def show_server(url, args):
  url += "/server"
  query = {}
  if args.server_id is not None:
    query["serverId"] = args.server_id;
  if len(query) > 0:
    encoded_query = urllib.urlencode(query)
    url += "?" + encoded_query
  return {"url":url, "postproc":open_url_and_show_response}

def show_trigger(url, args):
  url += "/trigger"
  query = {}
  if args.server_id is not None:
    query["serverId"] = args.server_id
  if args.host_id is not None:
    query["hostId"] = args.host_id
  if args.trigger_id is not None:
    query["triggerId"] = args.trigger_id
  if len(query) > 0:
    encoded_query = urllib.urlencode(query)
    url += "?" + encoded_query
  return {"url":url, "postproc":open_url_and_show_response}

def show_event(url, args):
  url += "/event"
  query = {}
  if args.sort is not None:
    orderDict = {"asc":hatohol.DATA_QUERY_OPTION_SORT_ASCENDING,
                 "desc":hatohol.DATA_QUERY_OPTION_SORT_DESCENDING}
    query["sortOrder"] = orderDict[args.sort]
  if args.max_number is not None:
    query["maximumNumber"] = args.max_number
  if args.start_id is not None:
    query["startId"] = args.start_id
  if len(query) > 0:
    encoded_query = urllib.urlencode(query)
    url += "?" + encoded_query
  return {"url":url, "postproc":open_url_and_show_response}

def show_item(url, args):
  url += "/item"
  return {"url":url, "postproc":open_url_and_show_response}

def show_host(url, args):
  url += "/host"
  query = {}
  if args.server_id is not None:
    query["serverId"] = args.server_id
  if args.host_id is not None:
    query["hostId"] = args.host_id
  if len(query) > 0:
    encoded_query = urllib.urlencode(query)
    url += "?" + encoded_query
  return {"url":url, "postproc":open_url_and_show_response}

def show_action(url, options):
  url += "/action"
  return {"url":url, "postproc":open_url_and_show_response}

def add_action(url, args):
  action_creator = ActionCreator(url)
  action_creator.add(args)
  url = action_creator.get_url()
  encoded_query = action_creator.get_encoded_query()
  return {"url":url, "postproc":open_url_and_show_response,
          "encoded_query":encoded_query}

def del_action(url, args):
  url = url + "/action/" + args.action_id
  req = urllib2.Request(url)
  req.get_method = lambda: 'DELETE'
  return {"url":req, "postproc":open_url_and_show_response}

def show_user(url, args):
  url += "/user"
  return {"url":url, "postproc":open_url_and_show_response}

def add_user(url, args):
  user_creator = UserCreator(url)
  user_creator.add(args)
  url = user_creator.get_url()
  encoded_query = user_creator.get_encoded_query()
  return {"url":url, "postproc":open_url_and_show_response,
          "encoded_query":encoded_query}

def del_user(url, args):
  url = url + "/user/" + args.user_id
  req = urllib2.Request(url)
  req.get_method = lambda: 'DELETE'
  return {"url":req, "postproc":open_url_and_show_response}

command_map = {
  "test":do_test,
  "show-server":show_server,
  "show-event":show_event,
  "show-trigger":show_trigger,
  "show-item":show_item,
  "show-host":show_host,
  "show-action":show_action,
  "add-action":add_action,
  "del-action":del_action,
  "show-user":show_user,
  "add-user":add_user,
  "del-user":del_user,
}

def main(arg_list=None, exec_postproc=True):
  parser = argparse.ArgumentParser(description="Hatohol Voyager")
  parser.add_argument("--server", type=parse_server_arg, dest="server_url",
                      metavar="SERVER[:PORT]",
                      default="%s:%d" % (DEFAULT_SERVER, DEFAULT_PORT))
  subparsers = parser.add_subparsers(help="Sub commands", dest="sub_command")

  # test
  sub_server = subparsers.add_parser("test")

  # server
  sub_server = subparsers.add_parser("show-server")
  sub_server.add_argument("server_id", type=int, nargs="?",
                          help="get a server only with server ID")
  # trigger
  sub_trigger = subparsers.add_parser("show-trigger")
  sub_trigger.add_argument("server_id", type=int, nargs="?")
  sub_trigger.add_argument("host_id", type=int, nargs="?")
  sub_trigger.add_argument("trigger_id", type=int, nargs="?")

  # event
  sub_event = subparsers.add_parser("show-event")
  sub_event.add_argument("--sort", choices=["asc", "desc"])
  sub_event.add_argument("-n", "--max-number", type=int)
  sub_event.add_argument("--start-id", type=int)

  # item
  sub_item = subparsers.add_parser("show-item")

  # host
  sub_host = subparsers.add_parser("show-host")
  sub_host.add_argument("server_id", type=int, nargs="?")
  sub_host.add_argument("host_id", type=int, nargs="?")

  # action
  sub_action = subparsers.add_parser("show-action")

  # action (add)
  sub_action = subparsers.add_parser("add-action")
  ActionCreator.setup_arguments(sub_action)

  # action (delete)
  sub_action = subparsers.add_parser("del-action")
  sub_action.add_argument("action_id")

  # user (show)
  sub_action = subparsers.add_parser("show-user")

  # user (add)
  sub_user = subparsers.add_parser("add-user")
  UserCreator.setup_arguments(sub_user)

  # user (delete)
  sub_user = subparsers.add_parser("del-user")
  sub_user.add_argument("user_id")

  args = parser.parse_args(arg_list)
  cmd_ctx = command_map[args.sub_command](args.server_url, args)
  if not exec_postproc:
    return cmd_ctx
  cmd_ctx["postproc"](cmd_ctx)
