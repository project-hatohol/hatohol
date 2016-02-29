=========================
GET Item
=========================

Return a list of `Item object`_ aggregated by the Hatohol server from each
monitoring servers.

Request
=======

Path
----
.. list-table::
   :header-rows: 1

   * - URL
   * - /item

Parameters
----------
.. list-table::
   :header-rows: 1

   * - Parameter
     - Brief
     - Condition
   * - fmt
     - Specifies the format of the returned data. "json" or "jsonp" are valid.
       If this parameter is omitted, the default value is "json".
     - | Optional for JSON ("json").
       | Mandatory for JSONP ("jsonp").
   * - callback
     - The name of the returned JSONP object.
     - | N/A for JSON.
       | Mandatory for JSONP.
   * - limit
     - Specifies a limit for the number of items to retrieve.
     - Optional
   * - offset
     - Specifies the number of items to skip before returning items.
     - Optional
   * - serverId
     - Specifies a monitoring server's ID to retrieve items belong to it.
     - Optional
   * - hostgroupId
     - Specifies a host group ID to retrieve items belong to it.
     - Optional
   * - hostId
     - Specifies a host ID to retrieve items belong to it.
     - Optional
   * - itemGroupName
     - Specifies a name of the item group to retrieve items that belong to it.
     - Optional
   * - itemId
     - Specifies an ID of an item to retrieve.
     - Optional

Response
========

Response structure
------------------
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
     - 0 on success, non-0 error code otherwise.
     - Always
   * - errorMessage
     - String
     - An error message.
     - False
   * - totalNumberOfItems
     - Number
     - The total number of items in the Hatohol's DB.
     - True
   * - numberOfItems
     - Number
     - The number of items in the `items` array.
     - True
   * - items
     - Array
     - Array of `Item object`_.
     - True
   * - itemGroups
     - Array
     - Array of `ItemGroup object`_.
     - True
   * - servers
     - Object
     - List of :ref:`server-object`. Keys for each :ref:`server-object` are
       server IDs which corresponds to serverId values in `Item object`_.
     - True

.. note:: [Condition] Always: always, True: only when result is True, False: only when result is False.

Item object
~~~~~~~~~~~~~~~~~~

`Item object`_ represents a monitoring item (such as CPU load, memory usage and
so on) which is fetched from a monitored host. This concept is inspired by
Zabbix.

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
   * - itemGroupNames
     - Array of string
     - The item group names.
   * - unit
     - String
     - The unit of the item.
   * - valueType
     - Number
     - A `value type`_ of the item.

Value type
~~~~~~~~~~~~~~~~~~

An item can be one of these types.

.. list-table::

   * - 0
     - ITEM_INFO_VALUE_TYPE_UNKNOWN
   * - 1
     - ITEM_INFO_VALUE_TYPE_FLOAT
   * - 2
     - ITEM_INFO_VALUE_TYPE_INTEGER
   * - 3
     - ITEM_INFO_VALUE_TYPE_STRING

ItemGroup object
~~~~~~~~~~~~~~~~~~

`ItemGroup object`_ represents a group of items.

.. list-table::
   :header-rows: 1

   * - Key
     - Value type
     - Brief
   * - name
     - String
     - A name of the item group.

Example
-------------
.. code-block:: json

  {
    "apiVersion":4,
    "errorCode":0,
    "items":[
      {
        "id":"23295",
        "serverId":4,
        "hostId":"10084",
        "brief":"Processor load (15 min average per core)",
        "lastValueTime":1453713375,
        "lastValue":"0.0500",
        "itemGroupNames":["CPU"],
        "unit":"",
        "valueType":1
      },
      {
        "id":"23296",
        "serverId":4,
        "hostId":"10084",
        "brief":"Processor load (1 min average per core)",
        "lastValueTime":1453713376,
        "lastValue":"0.0000",
        "itemGroupNames":[],
        "unit":"",
        "valueType":1
      },
      {
        "id":"23297",
        "serverId":4,
        "hostId":"10084",
        "brief":"Processor load (5 min average per core)",
        "lastValueTime":1453713377,
        "lastValue":"0.0200",
        "itemGroupNames":["CPU","Memory"],
        "unit":"",
        "valueType":1
      },
      {
        "id":"23298",
        "serverId":4,
        "hostId":"10084",
        "brief":"Context switches per second",
        "lastValueTime":1453713378,
        "lastValue":"145",
        "itemGroupNames":["CPU","Performance","Process"],
        "unit":"sps",
        "valueType":2
      },
      {
        "id":"23299",
        "serverId":4,
        "hostId":"10084",
        "brief":"CPU idle time",
        "lastValueTime":1453713379,
        "lastValue":"99.6800",
        "itemGroupNames":["CPU"],
        "unit":"%",
        "valueType":1
      },
    ],
    "itemGroups":[
      {
        "name":"Zabbix server"
      },
      {
        "name":"Zabbix agent"
      },
      {
        "name":"OS"
      },
      {
        "name":"Processes"
      },
      {
        "name":"General"
      },
      {
        "name":"CPU"
      },
      {
        "name":"Memory"
      },
      {
        "name":"Security"
      },
      {
        "name":"Network interfaces"
      },
      {
        "name":"Filesystems"
      }
    ],
    "numberOfItems":5,
    "totalNumberOfItems":180,
    "servers":{
      "4":{
        "name":"Zabbix",
        "nickname":"zabbix",
        "type":0,
        "ipAddress":"192.168.1.10",
        "baseURL":"",
        "hosts":{
          "10084":{
            "name":"Zabbix server"
          },
          "__SELF_MONITOR":{
            "name":"Zabbix_SELF"
          }
        },
        "groups":{
          "2":{
            "name":"Linux servers"
          },
          "4":{
            "name":"Zabbix servers"
          },
          "6":{
            "name":"HTTP servers"
          }
        }
      },
      "5":{
        "name":"HAPI2 Zabbix",
        "nickname":"HAPI2 Zabbix",
        "type":7,
        "ipAddress":"",
        "baseURL":"http://192.168.1.11/zabbix/api_jsonrpc.php",
        "uuid":"8e632c14-d1f7-11e4-8350-d43d7e3146fb",
        "hosts":{
          "10085":{
            "name":"debian"
          },
          "__SELF_MONITOR":{
            "name":"(self-monitor)"
          }
        },
        "groups":{
          "2":{
            "name":"Linux servers"
          },
          "4":{
            "name":"Zabbix servers"
          }
        }
      }
    }
  }
