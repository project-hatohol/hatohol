=========================
GET Event
=========================

Return a list of `Event object`_ aggregated by the Hatohol server from each
monitoring servers.

Request
=======

Path
----
.. list-table::
   :header-rows: 1

   * - URL
   * - /event

Parameters
----------
.. list-table::
   :header-rows: 1

   * - Parameter
     - Brief
     - Condition
   * - fmt
     - Specifies the format of the returned data. "json" or "jsonp" are valid.
       If this parameter is omitted, the default value is "json".
     - | Optional for JSON ("json").
       | Mandatory for JSONP ("jsonp").
   * - callback
     - The name of the returned JSONP object.
     - | N/A for JSON.
       | Mandatory for JSONP.
   * - limit
     - Specifies a limit for the number of events to retrieve.
     - Optional
   * - offset
     - Specifies the number of events to skip before returning events.
     - Optional
   * - serverId
     - Specifies a monitoring server's ID to retrieve events belong to it.
     - Optional
   * - hostgroupId
     - Specifies a host group ID to retrieve events belong to it.
     - Optional
   * - hostId
     - Specifies a host ID to retrieve events belong to it.
     - Optional
   * - limitOfUnifiedId
     - Specifies the unifiedId of the event to mark as the start point of
       pagination. Use with `limit` and `offset`.
     - Optional
   * - sortType
     - Any of `Sort type`_.
     - Optioanl
   * - sortOrder
     - Any of `Sort order`_. The default value is SORT_DONT_CARE.
     - If startId is specified, SORT_ASCENDING or SORT_DESCENDING has to be
       specified. Otherwise optional.
   * - minimumSeverity
     - Specifies the minimum :ref:`trigger-severity`.
     - Optioanl
   * - type
     - Any of `Event type`_.
     - Optional

Sort type
~~~~~~~~~~~~~~~~~~~
.. list-table::

    * - time
      - Sort by the time of the event.
    * - unifiedId
      - Sort by the unifiedId of the event.

Sort order
~~~~~~~~~~~~~~~~~~~
.. list-table::

   * - 0
     - SORT_DONT_CARE
   * - 1
     - SORT_ASCENDING
   * - 2
     - SORT_DESCENDING

Response
========
Response structure
------------------
.. list-table::
   :header-rows: 1

   * - Key
     - Value type
     - Brief
     - Condition
   * - apiVersion
     - Number
     - An API version of this URL.
       This document is written for version **4**.
     - Always
   * - errorCode
     - Number
     - 0 on success, non-0 error code otherwise.
     - Always
   * - errorMessage
     - String
     - An error message.
     - False
   * - haveIncident
     - Boolean
     - `true` when the incident tracking feature is enabled on the Hatohol
       server, otherwise `false`.
     - Always
   * - lastUnifiedEventId
     - Number
     - The ID (`unifiedId` in `Event object`_) of the latest event in
       Hatohol's DB.
     - True
   * - numberOfEvents
     - Number
     - The number of events in the `events` array.
     - True
   * - events
     - Array
     - The array of `Event object`_.
     - True
   * - servers
     - Object
     - List of :ref:`server-object`. Keys for each :ref:`server-object` are
       server IDs which corresponds to serverId values in `Event object`_.
     - True

.. note:: [Condition] Always: always, True: only when result is True, False: only when result is False.


Event object
~~~~~~~~~~~~~~~~~~~

`Event object`_ represents an event that is caught by a monitoring server or
Hatohol server. Events may be tied with :ref:`trigger-object` but it's not always
required.

.. list-table::
   :header-rows: 1

   * - Key
     - Value type
     - Brief
   * - unifiedId 
     - Number
     - A unified ID. This Id is unique in the Haothol DB.
   * - serverId
     - Number
     - A monitoring server ID.
   * - time
     - Number
     - A time of the event (in UNIX time).
   * - type
     - Number
     - An `Event type`_.
   * - triggerId
     - Number
     - The trigger ID.
   * - eventId
     - Number
     - The event ID in the monitoring server. It may not unique in the Hatohol DB.
   * - status
     - Number
     - Any of `Event type`_.
   * - severity
     - Number
     - Any of :ref:`trigger-severity`.
   * - hostId
     - Number
     - A host ID.
   * - brief
     - String
     - A brief of the event.
   * - extendedInfo
     - String
     - Additional data depend on the monitoring system.

Event type
~~~~~~~~~~~~~~~~~~~
.. list-table::

   * - 0
     - EVENT_TYPE_GOOD
   * - 1
     - EVENT_TYPE_BAD
   * - 2
     - EVENT_TYPE_UNKNOWN
   * - 3
     - EVENT_TYPE_NOTIFICATION

Example
-------------
.. code-block:: json

  {
    "apiVersion":4,
    "errorCode":0,
    "lastUnifiedEventId":374,
    "haveIncident":true,
    "events":[
      {
        "unifiedId":374,
        "serverId":5,
        "time":1453716192,
        "type":0,
        "triggerId":"__CON_HAP2",
        "eventId":"",
        "status":0,
        "severity":4,
        "hostId":"__SELF_MONITOR",
        "brief":"HAP2 connection unavailable.",
        "extendedInfo":"",
        "incident":{
          "trackerId":1,
          "identifier":"374",
          "location":"",
          "status":"NONE",
          "priority":"",
          "assignee":"",
          "doneRatio":0,
          "createdAt":1453716132,
          "updatedAt":1453716132
        }
      },
      {
        "unifiedId":373,
        "serverId":4,
        "time":1453665087,
        "type":0,
        "triggerId":"13531",
        "eventId":"00000000000000008311",
        "status":0,
        "severity":3,
        "hostId":"10085",
        "brief":"Lack of available memory on server {HOST.NAME}",
        "extendedInfo":"{\"expandedDescription\":\"Lack of available memory on server debian\"}",
        "incident":{
          "trackerId":1,
          "identifier":"373",
          "location":"",
          "status":"NONE",
          "priority":"",
          "assignee":"",
          "doneRatio":0,
          "createdAt":1453665354,
          "updatedAt":1453665354
        }
      },
      {
        "unifiedId":372,
        "serverId":4,
        "time":1453645872,
        "type":0,
        "triggerId":"13525",
        "eventId":"00000000000000008305",
        "status":0,
        "severity":2,
        "hostId":"10085",
        "brief":"Disk I/O is overloaded on {HOST.NAME}",
        "extendedInfo":"{\"expandedDescription\":\"Disk I/O is overloaded on debian\"}",
        "incident":{
          "trackerId":1,
          "identifier":"372",
          "location":"",
          "status":"NONE",
          "priority":"",
          "assignee":"",
          "doneRatio":0,
          "createdAt":1453664932,
          "updatedAt":1453664932
        }
      }
    ],
    "numberOfEvents":3,
    "servers":{
      "4":{
        "name":"Zabbix",
        "nickname":"zabbix",
        "type":0,
        "ipAddress":"192.168.1.10",
        "baseURL":"",
        "hosts":{
          "10085":{
            "name":"debian"
          },
          "__SELF_MONITOR":{
            "name":"Zabbix_SELF"
          }
        },
        "groups":{
          "4":{
            "name":"Zabbix servers"
          },
          "6":{
            "name":"HTTP servers"
          }
        }
      },
      "5":{
        "name":"HAPI2 Zabbix",
        "nickname":"HAPI2 Zabbix",
        "type":7,
        "ipAddress":"",
        "baseURL":"http://192.168.1.11/zabbix/api_jsonrpc.php",
        "uuid":"8e632c14-d1f7-11e4-8350-d43d7e3146fb",
        "hosts":{
          "10084":{
            "name":"Zabbix server"
          },
          "__SELF_MONITOR":{
            "name":"(self-monitor)"
          }
        },
        "groups":{
          "2":{
            "name":"Linux servers"
          },
          "4":{
            "name":"Zabbix servers"
          }
        }
      }
    },
    "incidentTrackers":{
      "1":{
        "type":1,
        "nickname":"",
        "baseURL":"",
        "projectId":"",
        "trackerId":""
      }
    }
  }
