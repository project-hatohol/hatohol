=========================
Overview
=========================

Request
=======

Path
----
.. list-table::
   :header-rows: 1

   * - URL
     - Comments
   * - /overview
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
     - Optional
   * - callback
     - The name of returned JSONP object.
     - N/A
     - N/A
     - Mandatory

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
       This document is written for version **2**.
     - Always
   * - result
     - Boolean
     - True on success. Otherwise False and the reason is shown in the
       element: message.
     - Always
   * - message
     - String
     - Error message. This key is reply only when result is False.
     - False
   * - numberOfServers
     - Number
     - The number of hosts.
     - True
   * - serverStatus
     - Array
     - The array of `ServerStatus object`_.
       Servers that Hatotal connot communicate with are not included in this array.
       They are listed in the array: badServers.
     - True
   * - badServers
     - Array
     - An array of `BadServer object`_.
     - True

.. note:: [Condition] Always: always, True: only when result is True, False: only when result is False.

ServerStatus object
-----------------------------
.. list-table::
   :header-rows: 1

   * - Key
     - Value type
     - Brief
   * - serverId
     - Number
     - A server ID.
   * - serverHostName
     - String
     - A server host name.
   * - serverIpAddr
     - String
     - An IP address of the server.
   * - serverNickname
     - String
     - A nickname of the server.
   * - numberOfHosts
     - Number
     - The number of hosts.
   * - numberOfItems
     - Number
     - The number of items.
   * - numberOfTriggers
     - Number
     - The number of triggers.
   * - numberOfUsers
     - Number
     - | The number of users.
       | **Currently not implemented ('0' is returned)**.
   * - numberOfOnlineUsers
     - Number
     - | The number of online users.
       | **Currently not implemented ('0' is returned)**.
   * - numberOfMonitoredItemsPerSecond
     - Number
     - | The number of monitored items per second.
       | **Currently not implemented ('0' is returned)**.
   * - hostGroups
     - Object
     - List of `HostGroup object`_. Keys for each `HostGroup object`_ are host group IDs. 
   * - systemStatus
     - Array
     - The array of `SystemStatus object`_.
   * - hostStatus
     - Array
     - The array of `HostStatus object`_.

HostGroup object
-------------------
.. list-table::
   :header-rows: 1

   * - Key
     - Value type
     - Brief
   * - name
     - String
     - A name of the host group.

.. note:: **Currently Hatohol server doesn't support host group. The host group named 'No group' is always returned.**

SystemStatus object
-------------------
.. list-table::
   :header-rows: 1

   * - Key
     - Value type
     - Brief
   * - hostGroupId
     - Number
     - A host groud ID.
   * - severity
     - Number
     - A :ref:`ref-trigger-severity`.
   * - numberOfHosts
     - Number
     - The number of hosts whose host group ID and serverity consist with hostGroupId and serverity in this object.

HostStatus object
-----------------
.. list-table::
   :header-rows: 1

   * - Key
     - Value type
     - Brief
   * - hostGroupId
     - Number
     - A host groud ID.
   * - numberOfGoodHosts
     - Number
     - The number of good hosts whose host group ID is hostGroupId in this object.
   * - numberOfBadHosts
     - Number
     - A number of bad hosts whose host group ID is hostGroupId in this object.

BadServer object
-----------------------------
.. list-table::
   :header-rows: 1

   * - Key
     - Value type
     - Brief
   * - serverId
     - Number
     - A server ID.
   * - brief
     - String
     - A brief of the problem.
