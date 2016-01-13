=========================
GET-Trigger
=========================
 **Please refer when you Get a Trigger.**
Request
=======

Path
----
.. list-table::
   :header-rows: 1

   * - URL
     - Comments
   * - /trigger
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
     - If this parameter is omitted, the return format is json.
     - Optional 
     - Optional
   * - callback
     - The name of returned JSONP object.
     - N/A
     - N/A
     - Mandatory
   * - hostId
     - N/A
     - N/A
     - Optional 
     - Optional
   * - hostgroupId
     - N/A
     - N/A
     - Optional 
     - Optional
   * - limit
     - N/A
     - N/A
     - Optional 
     - Optional
   * - minimumSeverity
     - N/A
     - N/A
     - Optional 
     - Optional
   * - offset
     - N/A
     - N/A
     - Optional 
     - Optional
   * - serverId
     - N/A
     - N/A
     - Optional 
     - Optional
   * - status
     - N/A
     - N/A
     - Optional 
     - Optional

Response
========

Repsponse structure
-------------------
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
     - N/A
     - Always
   * - errorMessage
     - String
     - N/A
     - False
   * - triggers
     - Array
     - The array of `Trigger object`_.
     - True
   * - numberOfTriggers
     - Number
     - The number of triggers.
     - True
   * - totalNumberOfTriggers
     - Number
     - N/A
     - True
   * - servers
     - Array
     - List of `Server object`_. Keys for each `Server object`_ are server IDs which corresponds to serverId values in `Trigger object`_.
     - True

.. note:: [Condition] Always: always, True: only when result is True, False: only when result is False.

Trigger object
--------------
.. list-table::
   :header-rows: 1

   * - Key
     - Value type
     - Brief
   * - id
     - String
     - N/A
   * - status
     - Number
     - A `Trigger status`_.
   * - severity
     - Number
     - A `Trigger severity`_.
   * - lastChangeTime
     - Number
     - A last change time of the trigger.
   * - serverId
     - Number
     - A server ID.
   * - hostId
     - String
     - A host ID.
   * - brief
     - String
     - A brief of the trigger.
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
     - A nickname of the server.
   * - type
     - Number
     - A `Server type`_.
   * - ipAddress
     - String
     - An IP Address of the server.
   * - baseURL
     - String
     - N/A
   * - hosts
     - Object
     - List of `Host object`_. Keys for each `Host object`_ are host IDs which corresponds to hostId values in `Trigger object`_.
   * - groups
     - Array
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

Group object
-------------
.. list-table::
   :header-rows: 1

   * - Key
     - Value type
     - Brief
   * - name
     - String
     - N/A

Trigger status
--------------
.. list-table::

   * - 0
     - TRIGGER_STATUS_OK
   * - 1
     - TRIGGER_STATUS_PROBLEM

.. _ref-trigger-severity:

Trigger severity
----------------
.. list-table::

   * - 0
     - TRIGGER_SEVERITY_INFO
   * - 1
     - TRIGGER_SEVERITY_WARN

Server type
-------------
.. list-table::

   * - 0
     - Zabbix
   * - 1
     - Nagios
