=========================
GET-Overview
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
       This document is written for version **4**.
     - Always
   * - errorCode
     - Number
     - N/A
     - Always
   * - errorMessage
     - String
     - Error message. This key is reply only when result is False.
     - False
   * - numberOfServers
     - Number
     - The number of hosts.
     - True
   * - serverStatus
     - Object
     - The array of `ServerStatus object`_.
       Servers that Hatotal connot communicate with are not included in this array.
       They are listed in the array: badServers.
     - True
   * - numberOfGoodServer
     - Number
     - The number of good servers.
     - True
   * - numberOfBadServer
     - Number
     - The number of bad servers.
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
   * - numberOfBadHosts
     - Number
     - The number of bad hosts.
   * - numberOfBadTriggers
     - Number
     - The number of bad triggers.
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
   * - hostgroupId
     - Number
     - A host groud ID.
   * - severity
     - Number
     - A :ref:`ref-trigger-severity`.
   * - numberOfTriggers
     - Number
     - The number of triggers.

HostStatus object
-----------------
.. list-table::
   :header-rows: 1

   * - Key
     - Value type
     - Brief
   * - hostgroupId
     - Number
     - A host groud ID.
   * - numberOfGoodHosts
     - Number
     - The number of good hosts whose host group ID is hostGroupId in this object.
   * - numberOfBadHosts
     - Number
     - A number of bad hosts whose host group ID is hostGroupId in this object.


