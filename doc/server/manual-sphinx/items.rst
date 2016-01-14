=========================
GET-Item
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
     - If this parameter is omitted, the return format is json.
     - Optional
     - Optional
   * - callback
     - The name of returned JSONP object.
     - N/A
     - N/A
     - Mandatory
   * - limit
     - N/A
     - N/A
     - N/A
     - N/A
   * - offset
     - N/A
     - N/A
     - N/A
     - N/A

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
     - String
     - N/A
     - Always
   * - errorMessage
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
   * - applications
     - Object
     - N/A
     - N/A
   * - totalNumberOfItems
     - Number
     - N/A
     - N/A
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
   * - id
     - Number
     - N/A
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
   * - itemGroupName
     - String
     - The item group name.
   * - unit
     - String
     - The unit of the item.
   * - valueType
     - Number
     - A `value type`_ of the item.

Value type
----------
.. list-table::

   * - 0
     - ITEM_INFO_VALUE_TYPE_UNKNOWN
   * - 1
     - ITEM_INFO_VALUE_TYPE_FLOAT
   * - 2
     - ITEM_INFO_VALUE_TYPE_INTEGER
   * - 3
     - ITEM_INFO_VALUE_TYPE_STRING

Application object
------------------
.. list-table::
   :header-rows: 1

   * - Key
     - Value type
     - Brief
   * - name
     - String
     - A hostname of the server.

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
     - N/A
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
     - N/A
   * - groups
     - Object
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
     - A hostname of the hosts.

Group object
-------------
.. list-table::
   :header-rows: 1

   * - Key
     - Value type
     - Brief
   * - name
     - String
     - A grouotname of the grouop.

Server type
-------------
.. list-table::

   * - 0
     - Zabbix
   * - 1
     - Nagios
