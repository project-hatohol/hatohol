#!/usr/bin/env python
# coding: UTF-8
"""
  Copyright (C) 2015-2016 Project Hatohol

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

import MySQLdb
import time
from logging import getLogger
import datetime
from hatohol import hap
from hatohol import haplib
from hatohol import standardhap
from hatohol import hapcommon

logger = getLogger("hatohol.hap2_nagios_ndoutils:%s" % hapcommon.get_top_file_name())

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

    INITIAL_LAST_INFO = ""

    def __init__(self):
        self.__db = None
        self.__cursor = None

        self.__db_server = self.DEFAULT_SERVER
        self.__db_port = self.DEFAULT_PORT
        self.__db_name = self.DEFAULT_DATABASE
        self.__db_user = self.DEFAULT_USER
        self.__db_passwd = ""
        self.__time_offset = datetime.timedelta(seconds=time.timezone)
        self.__trigger_last_info = None

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
            logger.info("Use default connection parameters.")
        else:
            self.__db_server, self.__db_port, self.__db_name = \
                self.__parse_url(ms_info.url)
            self.__db_user = ms_info.user_name
            self.__db_passwd = ms_info.password

        logger.info("Try to connection: Sv: %s, DB: %s, User: %s" %
                     (self.__db_server, self.__db_name, self.__db_user))

        try:
            self.__db = MySQLdb.connect(host=self.__db_server,
                                        port=self.__db_port,
                                        db=self.__db_name,
                                        user=self.__db_user,
                                        passwd=self.__db_passwd)
            self.__cursor = self.__db.cursor()
        except MySQLdb.Error as (errno, msg):
            logger.error('MySQL Error [%d]: %s' % (errno, msg))
            raise hap.Signal

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
            port = int(server[colon_idx+1:])
            server = server[0:colon_idx]

        return server, port, database

    def __convert_to_nagios_time(self, hatohol_time):
        over_second = hatohol_time.split(".")[0]
        nag_datetime = datetime.datetime.strptime(over_second, '%Y%m%d%H%M%S')
        return nag_datetime.strftime("%Y-%m-%d %H:%M:%S")

    def __get_latest_statehistory_id(self):
        sql = "SELECT statehistory_id FROM nagios_statehistory " \
              "ORDER BY statehistory_id DESC LIMIT 1;"

        statehistory_id = None
        if self.__cursor.execute(sql):
            statehistory_id = str(self.__cursor.fetchall()[0][0])

        return statehistory_id

    def collect_hosts_and_put(self):
        sql = "SELECT host_object_id,display_name FROM nagios_hosts"
        self.__cursor.execute(sql)
        result = self.__cursor.fetchall()
        hosts = [{"hostId": str(hid), "hostName": name} for hid, name in result]
        self.divide_and_put_data(self.put_hosts, hosts)

    def collect_host_groups_and_put(self):
        sql = "SELECT hostgroup_object_id,alias FROM nagios_hostgroups"
        self.__cursor.execute(sql)
        result = self.__cursor.fetchall()
        groups = \
          [{"groupId": str(grid), "groupName": name} for grid, name in result]
        self.divide_and_put_data(self.put_host_groups, groups)

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
        self.divide_and_put_data(self.put_host_group_membership, membership)

    def collect_triggers_and_put(self, fetch_id=None, host_ids=None):

        if host_ids is not None and not self.__validate_object_ids(host_ids):
            logger.error("Invalid: host_ids: %s" % host_ids)
            # TODO by 15.09 (*1): send error
            # There's no definition to send error in HAPI2.
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

        all_triggers_should_send = lambda: fetch_id is None
        update_type = "ALL"
        if all_triggers_should_send():
            if self.__trigger_last_info is None:
                self.__trigger_last_info = self.get_last_info("trigger")

            if len(self.__trigger_last_info):
                nag_time = self.__convert_to_nagios_time(self.__trigger_last_info)
                sql += " WHERE status_update_time >= '%s'" % nag_time
                update_type = "UPDATED"

        self.__cursor.execute(sql)
        result = self.__cursor.fetchall()

        triggers = []
        for row in result:
            (trigger_id, state, update_time, msg, host_id, host_name) = row

            hapi_status, hapi_severity = \
              self.__parse_status_and_severity(state)

            hapi_time = hapcommon.conv_to_hapi_time(update_time,
                                                    self.__time_offset)
            triggers.append({
                "triggerId": str(trigger_id),
                "status": hapi_status,
                "severity": hapi_severity,
                "lastChangeTime": hapi_time,
                "hostId": str(host_id),
                "hostName": host_name,
                "brief": msg,
                "extendedInfo": ""
            })
        self.__trigger_last_info = \
            hapcommon.get_biggest_num_of_dict_array(triggers,
                                                    "lastChangeTime")
        put_empty_contents = True
        if fetch_id is None:
            put_empty_contents = False

        self.divide_and_put_data(self.put_triggers, triggers,
                           put_empty_contents,
                           update_type=update_type,
                           last_info=self.__trigger_last_info,
                           fetch_id=fetch_id)

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
              + "ON %s.object_id=%s.service_object_id " % (t0, t1) \
              + "INNER JOIN %s " % t2 \
              + "ON %s.host_object_id=%s.host_object_id" % (t1, t2)

        # Event range to select
        if last_info is not None:
            raw_last_info = last_info
        else:
            raw_last_info = self.get_cached_event_last_info()
            if not raw_last_info:
                latest_id = self.__get_latest_statehistory_id()
                if latest_id:
                    self.set_event_last_info(latest_id)
                else:
                    self.set_event_last_info("0")
                return

        if raw_last_info is not None \
            and raw_last_info != self.INITIAL_LAST_INFO:
            # The form of 'last_info' depends on a plugin. So the validation
            # of it cannot be completed in haplib.Receiver.__validate_arguments().
            # Since it is inserted into the SQL statement, we have to strictly
            # validate it here.
            last_cond = self.__extract_validated_event_last_info(raw_last_info)
            if last_cond is None:
                logger.error("Malformed last_info: '%s'",
                              str(raw_last_info))
                logger.error("Getting events was aborted.")
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

        logger.debug(sql)
        self.__cursor.execute(sql)
        result = self.__cursor.fetchall()

        events = []
        for (event_id, state, event_time, msg, \
             trigger_id, host_id, host_name) in result:

            hapi_event_type = self.EVENT_TYPE_MAP.get(state)
            if hapi_event_type is None:
                logger.warning("Unknown state: " + str(state))
                hapi_event_type = "UNKNOWN"

            hapi_status, hapi_severity = \
              self.__parse_status_and_severity(state)

            hapi_time = hapcommon.conv_to_hapi_time(event_time,
                                                    self.__time_offset)
            events.append({
                "eventId": str(event_id),
                "time": hapi_time,
                "type": hapi_event_type,
                "triggerId": str(trigger_id),
                "status": hapi_status,
                "severity": hapi_severity,
                "hostId": str(host_id),
                "hostName": host_name,
                "brief": msg,
                "extendedInfo": ""
            })

        put_empty_contents = True
        if fetch_id is None:
            put_empty_contents = False

        self.divide_and_put_data(self.put_events, events, put_empty_contents,
                                 fetch_id=fetch_id)


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
        if event_id < 0:
            event_id = None
        return event_id

    def __parse_status_and_severity(self, status):
        hapi_status = self.STATUS_MAP.get(status)
        if hapi_status is None:
            logger.warning("Unknown status: " + str(status))
            hapi_status = "UNKNOWN"

        hapi_severity = self.SEVERITY_MAP.get(status)
        if hapi_severity is None:
            logger.warning("Unknown status: " + str(status))
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
        haplib.BaseMainPlugin.__init__(self)
        Common.__init__(self)

    def hap_fetch_triggers(self, params, request_id):
        self.ensure_connection()
        # TODO: return FAILURE when connection fails
        self.get_sender().response("SUCCESS", request_id)
        fetch_id = params["fetchId"]
        host_ids = params.get("hostIds")
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
    Hap2NagiosNDOUtils().run()
