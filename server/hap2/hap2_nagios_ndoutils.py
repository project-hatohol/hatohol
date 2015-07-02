#!/usr/bin/env python
# coding: UTF-8
"""
  Copyright (C) 2015 Project Hatohol

  This file is part of Hatohol.

  Hatohol is free software: you can redistribute it and/or modify
  it under the terms of the GNU Lesser General Public License, version 3
  as published by the Free Software Foundation.

  Hatohol is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
  GNU Lesser General Public License for more details.

  You should have received a copy of the GNU Lesser General Public
  License along with Hatohol. If not, see
  <http://www.gnu.org/licenses/>.
"""

import sys
import MySQLdb
import time
import haplib
import standardhap
import logging

class Common:

    STATE_OK = 0
    STATE_WARNING = 1
    STATE_CRITICAL = 2

    STATUS_MAP = {STATE_OK: "OK", STATE_WARNING: "NG", STATE_CRITICAL: "NG"}
    SEVERITY_MAP = {
        STATE_OK: "INFO", STATE_WARNING: "WARNING", STATE_CRITICAL: "CRITICAL"}
    EVENT_TYPE_MAP = {
        STATE_OK: "GOOD", STATE_WARNING: "BAD", STATE_CRITICAL: "BAD"}

    DEFAULT_SERVER = "localhost"
    DEFAULT_PORT = 3306
    DEFAULT_DATABASE = "ndoutils"
    DEFAULT_USER = "root"

    def __init__(self):
        self.__db = None
        self.__cursor = None

        self.__db_server = self.DEFAULT_SERVER
        self.__db_port = self.DEFAULT_PORT
        self.__db_name = self.DEFAULT_DATABASE
        self.__db_user = self.DEFAULT_USER
        self.__db_passwd = ""

    def close_connection(self):
        if self.__cursor is not None:
            self.__cursor.close()
            self.__cursor = None
        if self.__db is not None:
            self.__db.close()
            self.__db = None

    def ensure_connection(self):
        if self.__db is not None:
            return

        # load MonitoringServerInfo
        ms_info = self.get_ms_info()
        if ms_info is None:
            logging.info("Use default connection parameters.")
        else:
            self.__db_server, self.__db_port, self.__db_name = \
                self.__parse_url(ms_info.url)
            self.__db_user = ms_info.user_name
            self.__db_passwd = ms_info.password

        logging.info("Try to connection: Sv: %s, DB: %s, User: %s" %
                     (self.__db_server, self.__db_name, self.__db_user))

        try:
            self.__db = MySQLdb.connect(host=self.__db_server,
                                        port=self.__db_port,
                                        db=self.__db_name,
                                        user=self.__db_user,
                                        passwd=self.__db_passwd)
            self.__cursor = self.__db.cursor()
        except MySQLdb.Error as (errno, msg):
            logging.error('MySQL Error [%d]: %s' % (errno, msg))
            raise haplib.Signal

    def __parse_url(self, url):
        # [URL] SERVER_IP:PORT/DATABASE
        server = self.DEFAULT_SERVER
        port = self.DEFAULT_PORT
        database = self.DEFAULT_DATABASE

        slash_idx = url.find("/")
        if slash_idx >= 0:
            database = url[slash_idx+1:]
            if slash_idx != 0:
                server = url[0:slash_idx]
        else:
            server = url

        colon_idx = server.find(":")
        if colon_idx >= 0:
            port = server[colon_idx+1:]
            server = server[0:colon_idx]

        return server, port, database

    def collect_hosts_and_put(self):
        sql = "SELECT host_object_id,display_name FROM nagios_hosts"
        self.__cursor.execute(sql)
        result = self.__cursor.fetchall()
        hosts = [{"hostId": str(hid), "hostName": name} for hid, name in result]
        self.put_hosts(hosts)

    def collect_host_groups_and_put(self):
        sql = "SELECT hostgroup_object_id,alias FROM nagios_hostgroups"
        self.__cursor.execute(sql)
        result = self.__cursor.fetchall()
        groups = \
          [{"groupId": str(grid), "groupName": name} for grid, name in result]
        self.put_host_groups(groups)

    def collect_host_group_membership_and_put(self):
        tbl0 = "nagios_hostgroup_members"
        tbl1 = "nagios_hostgroups"
        sql = "SELECT " \
            + "%s.host_object_id, " % tbl0 \
            + "%s.hostgroup_object_id " % tbl1 \
            + "FROM %s INNER JOIN %s " % (tbl0, tbl1) \
            + "ON %s.hostgroup_id=%s.hostgroup_id" % (tbl0, tbl1)
        self.__cursor.execute(sql)
        result = self.__cursor.fetchall()
        members = {}
        for host_id, group_id in result:
            members_for_host = members.get(host_id)
            if members_for_host is None:
                members[host_id] = []
            members[host_id].append(str(group_id))

        membership = []
        for host_id, group_list in members.items():
            membership.append({"hostId": str(host_id), "groupIds": group_list})
        self.put_host_group_membership(membership)

    def collect_triggers_and_put(self, fetch_id=None, host_ids=None):

        if host_ids is not None and not self.__validate_object_ids(host_ids):
            logging.error("Invalid: host_ids: %s" % host_ids)
            # TODO by 15.09 (*1): send error
            # There's no definition to send error in HAPI 2.0.
            # We have to extend the specification to enable this.
            return

        t0 = "nagios_services"
        t1 = "nagios_servicestatus"
        t2 = "nagios_hosts"
        sql = "SELECT " \
              + "%s.service_object_id, " % t0 \
              + "%s.current_state, " % t1 \
              + "%s.status_update_time, " % t1 \
              + "%s.output, " % t1 \
              + "%s.host_object_id, " % t2 \
              + "%s.display_name " % t2 \
              + "FROM %s INNER JOIN %s " % (t0, t1) \
              + "ON %s.service_object_id=%s.service_object_id " % (t0, t1) \
              + "INNER JOIN %s " % t2 \
              + "ON %s.host_object_id=%s.host_object_id" % (t0, t2)

        if host_ids is not None:
            in_cond = "','".join(host_ids)
            sql += " WHERE %s.host_object_id in ('%s')" % (t2, in_cond)

        # NOTE: The update time in the output is renewed every status
        #       check in Nagios even if the value is not changed.
        # TODO by 15.09:
        #   We should has the previous result and compare it here in order to
        #   improve performance. Or other columns such as last_state_change in
        #   nagios_servicestatus might be used.
        self.__cursor.execute(sql)
        result = self.__cursor.fetchall()

        triggers = []
        for row in result:
            (trigger_id, state, update_time, msg, host_id, host_name) = row

            hapi_status, hapi_severity = \
              self.__parse_status_and_severity(state)

            triggers.append({
                "triggerId": str(trigger_id),
                "status": hapi_status,
                "severity": hapi_severity,
                # TODO: take into acount the timezone
                "lastChangeTime": update_time.strftime("%Y%m%d%H%M%S"),
                "hostId": str(host_id),
                "hostName": host_name,
                "brief": msg,
                "extendedInfo": ""
            })
        # TODO by 15.09: Implement UPDATED mode.
        update_type = "ALL"
        self.put_triggers(triggers, update_type=update_type, fetch_id=fetch_id)

    def collect_events_and_put(self, fetch_id=None, last_info=None,
                               count=None, direction="ASC"):
        t0 = "nagios_statehistory"
        t1 = "nagios_services"
        t2 = "nagios_hosts"

        sql = "SELECT " \
              + "%s.statehistory_id, " % t0 \
              + "%s.state, " % t0 \
              + "%s.state_time, " % t0 \
              + "%s.output, " % t0 \
              + "%s.service_object_id, " % t1 \
              + "%s.host_object_id, " % t2 \
              + "%s.display_name " % t2 \
              + "FROM %s INNER JOIN %s " % (t0, t1) \
              + "ON %s.statehistory_id=%s.service_object_id " % (t0, t1) \
              + "INNER JOIN %s " % t2 \
              + "ON %s.host_object_id=%s.host_object_id" % (t1, t2)

        # Event range to select
        if last_info is not None:
            raw_last_info = last_info
        else:
            raw_last_info = self.get_cached_event_last_info()

        if raw_last_info is not None:
            # The form of 'last_info' depends on a plugin. So the validation
            # of it cannot be completed in haplib.Utils.validate_arguments().
            # Since it is inserted into the SQL statement, we have to strictly
            # validate it here.
            last_cond = self.__extract_validated_event_last_info(raw_last_info)
            if last_cond is None:
                logging.error("Malformed last_info: '%s'",
                              str(raw_last_info))
                logging.error("Getting events was aborted.")
                # TODO by 15.09: notify error to the caller
                # See  also TODO (*1)
                return
            # TODO: Fix when direction is 'DESC'
            sql += " WHERE %s.statehistory_id>%s" % (t0, last_cond)

        # Direction
        if direction in ["ASC", "DESC"]:
            sql += " ORDER BY %s.statehistory_id %s" % (t0, direction)

        # The number of records
        if count is not None:
            sql += " LIMIT %d" % count

        logging.debug(sql)
        self.__cursor.execute(sql)
        result = self.__cursor.fetchall()

        events = []
        for (event_id, state, event_time, msg, \
             trigger_id, host_id, host_name) in result:

            hapi_event_type = self.EVENT_TYPE_MAP.get(state)
            if hapi_event_type is None:
                log.warning("Unknown status: " + str(status))
                hapi_event_type = "UNKNOWN"

            hapi_status, hapi_severity = \
              self.__parse_status_and_severity(state)

            events.append({
                "eventId": str(event_id),
                "time": event_time.strftime("%Y%m%d%H%M%S"),
                "type": hapi_event_type,
                "triggerId": trigger_id,
                "status": hapi_status,
                "severity": hapi_severity,
                "hostId": str(host_id),
                "hostName": host_name,
                "brief": msg,
                "extendedInfo": ""
            })
        self.put_events(events, fetch_id=fetch_id)


    def __validate_object_ids(self, host_ids):
        for host_id in host_ids:
            if not self.__validate_object_id(host_id):
                return False
        return True

    def __validate_object_id(self, host_id):
        try:
            obj_id = int(host_id)
        except:
            return False
        return obj_id >= 0

    def __extract_validated_event_last_info(self, last_info):
        event_id = None
        try:
            event_id = int(last_info)
        except:
            pass
        if event_id <= 0:
            event_id = None
        return event_id

    def __parse_status_and_severity(self, status):
        hapi_status = self.STATUS_MAP.get(status)
        if hapi_status is None:
            logging.warning("Unknown status: " + str(status))
            hapi_status = "UNKNOWN"

        hapi_severity = self.SEVERITY_MAP.get(status)
        if hapi_severity is None:
            logging.warning("Unknown status: " + str(status))
            hapi_severity = "UNKNOWN"

        return (hapi_status, hapi_severity)


class Hap2NagiosNDOUtilsPoller(haplib.BasePoller, Common):

    def __init__(self, *args, **kwargs):
        haplib.BasePoller.__init__(self, *args, **kwargs)
        Common.__init__(self)

    def poll_setup(self):
        self.ensure_connection()

    def poll_hosts(self):
        self.collect_hosts_and_put()

    def poll_host_groups(self):
        self.collect_host_groups_and_put()

    def poll_host_group_membership(self):
        self.collect_host_group_membership_and_put()

    def poll_triggers(self):
        self.collect_triggers_and_put()

    def poll_events(self):
        self.collect_events_and_put()

    def on_aborted_poll(self):
        self.reset()
        self.close_connection()


class Hap2NagiosNDOUtilsMain(haplib.BaseMainPlugin, Common):
    def __init__(self, *args, **kwargs):
        haplib.BaseMainPlugin.__init__(self, kwargs["transporter_args"])
        Common.__init__(self)

    def hap_fetch_triggers(self, params, request_id):
        self.ensure_connection()
        # TODO: return FAILURE when connection fails
        self.get_sender().response("SUCCESS", request_id)
        fetch_id = params["fetchId"]
        host_ids = params["hostIds"]
        self.collect_triggers_and_put(fetch_id, host_ids)

    def hap_fetch_events(self, params, request_id):
        self.ensure_connection()
        # TODO: return FAILURE when connection fails
        self.get_sender().response("SUCCESS", request_id)
        self.collect_events_and_put(params["fetchId"], params["lastInfo"],
                                    params["count"], params["direction"])


class Hap2NagiosNDOUtils(standardhap.StandardHap):
    def create_main_plugin(self, *args, **kwargs):
        return Hap2NagiosNDOUtilsMain(*args, **kwargs)

    def create_poller(self, *args, **kwargs):
        return Hap2NagiosNDOUtilsPoller(self, *args, **kwargs)


if __name__ == '__main__':
    hap = Hap2NagiosNDOUtils()
    hap()
