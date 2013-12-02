=========================
Item
=========================

Request
=======

Path
----
.. list-table::
   :header-rows: 1

   * - URL
     - Comments
   * - /item
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
   * - numberOfItems
     - Number
     - The number of triggers.
     - True
   * - items
     - Array
     - The array of `Item object`_.
     - True
   * - servers
     - Object
     - List of `Server object`_. Keys for each `Server object`_ are server IDs which corresponds to serverId values in `Item object`_.
     - True

.. note:: [Condition] Always: always, True: only when result is True, False: only when result is False.

Item object
-------------
.. list-table::
   :header-rows: 1

   * - Key
     - Value type
     - Brief
   * - serverId
     - Number
     - A server ID.
   * - hostId
     - Number
     - A host ID.
   * - brief
     - String
     - A brief of the item.
   * - lastValueTime
     - Number
     - A time of the last value.
   * - lastValue
     - String
     - The last value.
   * - prevValue
     - String
     - The previous value.
   * - itemGroupName
     - String
     - The item group name.

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
   * - type
     - Number
     - A Server type.
   * - ipAddress
     - String
     - An IP Address of the server.
