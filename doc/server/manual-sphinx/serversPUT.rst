=========================
PUT-Server
=========================
 **Please refer when you Update a Monitoring Server.**

Request
=======

Path
----
.. list-table::
   :header-rows: 1

   * - URL
     - Comments
   * - /server/<serverId>
     - N/A


Parameters
----------
.. list-table::
   :header-rows: 1

   * - Parameter
     - Value
     - Comments
   * - hostName
     - N/A
     - N/A
   * - ipAddress
     - N/A
     - N/A
   * - nickname
     - N/A
     - N/A
   * - password
     - N/A
     - N/A
   * - pollingInterval
     - N/A
     - N/A
   * - port
     - N/A
     - N/A
   * - retryInterval
     - N/A
     - N/A
   * - type
     - N/A
     - N/A
   * - userName
     - N/A
     - N/A

Server type
-------------
.. list-table::

   * - 0
     - Zabbix
   * - 1
     - Nagios

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
   * - id
     - Number
     - Server id.
     - True

.. note:: [Condition] Always: always, True: only when result is True, False: only when result is False.

