=========================
GET-Event
=========================

Request
=======

Path
----
.. list-table::
   :header-rows: 1

   * - URL
     - Comments
   * - /event
     - N/A

Parameters
----------
.. list-table::
   :header-rows: 1

   * - Parameter
     - Value
     - Comments
     - JSON
     - JSONP
   * - fmt
     - json or jsonp
     - This parameter is omitted, the return format is json.
     - Optional 
     - <-
   * - callback
     - The name of returned JSONP object.
     - N/A
     - N/A
     - Mandatory
   * - limit
     - N/A
     - N/A
     - N/A
     - N/A
   * - offset
     - N/A
     - N/A
     - N/A
     - N/A
   * - limitOfUnifiedId
     - N/A 
     - N/A
     - N/A
     - N/A
   * - sortType
     - N/A
     - N/A
     - N/A
     - N/A
   * - sortOrder
     - Any of `Sort order`_.
     - The default value is SORT_DONT_CARE.
     - If startId is specified, SORT_ASCENDING or SORT_DESCENDING has to be
       specified. Otherwise optional.
     - <-
   * - minimumSeverity
     - N/A
     - N/A
     - N/A
     - N/A
   * - type
     - N/A
     - N/A
     - N/A
     - N/A

Sort order
----------
.. list-table::

   * - 0
     - SORT_DONT_CARE
   * - 1
     - SORT_ASCENDING
   * - 2
     - SORT_DESCENDING

Response
========
Repsponse structure
-------------------
-----------
Zabbix
-----------
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
   * - haveIncident
     - Boolean
     - True on success. Otherwise False and the reason is shown in the
       element: errorMessage.
     - Always
   * - errorMessage
     - String
     - Error message. This key is reply only when result is False.
     - False
   * - errorCode
     - Number
     - N/A
     - N/A
   * - lastUnifiedEventlId
     - Number
     - N/A
     - N/A
   * - numberOfEvents
     - Number
     - The number of events.
     - True
   * - events
     - Array
     - The array of `Event object`_.
     - True
   * - servers
     - Object
     - List of `Server object`_. Keys for each `Server object`_ are server IDs which corresponds to serverId values in `Event object`_.
     - True

.. note:: [Condition] Always: always, True: only when result is True, False: only when result is False.


Event object
-------------
.. note:: Only the event of the triggers are returned in the current version.
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
     - A server ID.
   * - time
     - Number
     - A time of the event.
   * - type
     - Number
     - An `Event type`_.
   * - triggerId
     - Number
     - The trigger ID.
   * - eventId
     - Number
     - The event ID.
   * - status
     - Number
     - N/A
   * - severity
     - Number
     - N/A
   * - hostId
     - Number
     - A host ID.
   * - brief
     - String
     - A brief of the event.
   * - extendedInfo
     - String
     - N/A

Server object
-------------
.. list-table::
   :header-rows: 1

   * - Key
     - Value type
     - Brief
   * - name
     - String
     - A hostname of the server.
   * - nickname
     - String
     - N/A
   * - type
     - Number
     - N/A
   * - ipAddress
     - String
     - An IP Address of the server.
   * - baseURL
     - String
     - N/A
   * - hosts
     - Object
     - List of `Host object`_. Keys for each `Host object`_ are host IDs which corresponds to hostId values in `Event object`_.
   * - groups
     - Object
     - N/A

Host object
-------------
.. list-table::
   :header-rows: 1

   * - Key
     - Value type
     - Brief
   * - name
     - String
     - A hostname of the host.
   * - type
     - Number
     - A `Server type`_.


Group object
-------------
.. list-table::
   :header-rows: 1

   * - Key
     - Value type
     - Brief
   * - name
     - String
     - A hostname of the host.

Event type
-------------
.. list-table::

   * - 0
     - EVENT_TYPE_GOOD
   * - 1
     - EVENT_TYPE_BAD
   * - 2
     - EVENT_TYPE_UNKNOWN
   * - 3
     - EVENT_TYPE_NOTIFICATION

Server type
-------------
.. list-table::

   * - 0
     - Zabbix
   * - 1
     - Nagios
