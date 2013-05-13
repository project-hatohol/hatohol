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
     - A
   * - result
     - Boolean
     - True on success. Otherwise False and the reason is shown in the
       element: message.
     - A
   * - message
     - String
     - Error message. This key is reply only when result is False.
     - F
   * - numberOfServers
     - Number
     - The number of servers.
     - T
   * - servers
     - Array
     - The array of `Server object`_.
     - T

.. note:: [Condition] A: always, T: only when result is True, F: only when result is False.

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

