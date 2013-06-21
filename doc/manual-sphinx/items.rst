=========================
Item
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
     - /items.json
   * - JSONP
     - /items.jsonp

Parameters
----------
.. list-table::
   :header-rows: 1

   * - Parameter
     - Brief
     - JSON
     - JSONP
   * - callback
     - A callback function name for a JSONP format.
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
   * - numberOfItems
     - Number
     - The number of triggers.
     - True
   * - items
     - Array
     - The array of `Item object`_.
     - True

.. note:: [Condition] A: always, T: only when result is True, F: only when result is False.

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
