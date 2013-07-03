=========================
Overview
=========================

Request
=======

Path
----
.. list-table::
   :header-rows: 1

   * - Format
     - URL
   * - JSON
     - /overview.json
   * - JSONP
     - /overview.jsonp

Parameters
----------
.. list-table::
   :header-rows: 1

   * - Parameter
     - JSON
     - JSONP
   * - callback
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
       This document is written for version **1**.
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
   * - numberOfHosts
     - Number
     - The number of hosts.
     - True
   * - numberOfItems
     - Number
     - The number of items.
     - True
   * - numberOfTriggers
     - Number
     - The number of triggers.
     - True
   * - numberOfUsers
     - Number
     - | The number of users.
       | **Currently not implemented ('0' is returned)**.
     - True
   * - numberOfOnlineUsers
     - Number
     - | The number of online users.
       | **Currently not implemented ('0' is returned)**.
     - True
   * - numberOfMonitoredItemsPerSecond
     - Number
     - | The number of monitored items per second.
       | **Currently not implemented ('0' is returned)**.
     - True
   * - hostGroups
     - Array
     - The array of `HostGroup object`_.
     - True
   * - systemStatus
     - Array
     - The array of `SystemStatus object`_.
     - True
   * - hostStatus
     - Array
     - The array of `HostStatus object`_.
     - True

.. note:: [Condition] Always: always, True: only when result is True, False: only when result is False.

HostGroup object
-------------------
.. list-table::
   :header-rows: 1

   * - Key
     - Value type
     - Brief
   * - hostGroupId
     - Number
     - A host groud ID.
   * - hostGroupName
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
