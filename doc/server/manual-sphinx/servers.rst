=========================
Servers
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
     - /servers.json
   * - JSONP
     - /servers.jsonp

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
     - A Server type.
   * - hostName
     - String
     - A host name.
   * - ipAddress
     - String
     - An IP Address of the server.
   * - nickname
     - String
     - Arbitrary nickname of the server.

