=========================
GET-Action
=========================
 **Please refer when you Get an Action.**

Request
=======

Path
----
.. list-table::
   :header-rows: 1

   * - URL
     - Comments
   * - /action
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
   * - type
     - N/A
     - If you want `Incident tracking`, it value must be `2`.
     - Optional
     - Optional

Parameter type
--------------
.. list-table::

   * - N/A
     - Action
   * - 2
     - Incident tracking

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
   * - numberOfActions
     - Number
     - N/A
     - True
   * - actions
     - Array
     - N/A
     - True
   * - servers
     - Array
     - N/A
     - True

.. note:: [Condition] Always: always, True: only when result is True, False: only when result is False.

Action object
--------------
.. list-table::
   :header-rows: 1

   * - Key
     - Value type
     - Brief
   * - actionId
     - Number
     - N/A
   * - enableBits
     - Number
     - N/A
   * - serverId
     - Number
     - A server ID.
   * - hostId
     - String
     - A host ID.
   * - hostgroupId
     - String
     - A host ID.
   * - triggerId
     - String
     - N/A
   * - triggerStatus
     - Number
     - N/A
   * - triggerSeverity
     - Number
     - N/A
   * - triggerSeverityComparatorType
     - Number
     - N/A
   * - type
     - Number
     - N/A
   * - workingDirectory
     - String
     - N/A
   * - command
     - String
     - N/A
   * - timeout
     - Number
     - N/A
   * - ownerUserId
     - Number
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
   * - triggers
     - Object
     - N/A
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

Trigger object
--------------
.. list-table::
   :header-rows: 1

   * - Key
     - Value type
     - Brief
   * - brief
     - String
     - N/A

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
