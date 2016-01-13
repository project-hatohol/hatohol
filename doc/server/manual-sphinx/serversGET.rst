=========================
GET-Server
=========================
 **Please refer when you Get a Monitoring Server.**

Request
=======

Path
----
.. list-table::
   :header-rows: 1

   * - URL
     - Comments
   * - /server
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
   * - numberOfServers
     - Number
     - The number of servers.
     - True
   * - servers
     - Array
     - The array of `Server object`_.
     - True

.. note:: [Condition] Always: always, True: only when result is True, False: only when result is False.

Server object
-------------
.. list-table::
   :header-rows: 1

   * - Key
     - Value type
     - Brief
   * - id
     - Number
     - A unique server ID.
   * - type
     - Number
     - Any of `Server type`_.
   * - hostName
     - String
     - A host name.
   * - ipAddress
     - String
     - An IP Address of the server.
   * - nickname
     - String
     - Arbitrary nickname of the server.
   * - port
     - Number
     - N/A
   * - pollingInterval
     - Number
     - N/A
   * - retryInterval
     - Number
     - N/A
   * - baseURL
     - String
     - N/A
   * - extendedInfo
     - String
     - N/A
   * - userName
     - String
     - N/A
   * - password
     - String
     - N/A
   * - dbName
     - String
     - N/A

Server type
-------------
.. list-table::

   * - 0
     - Zabbix
   * - 1
     - Nagios
