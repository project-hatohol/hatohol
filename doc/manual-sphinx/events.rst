=========================
Event
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
     - /events.json
   * - JSONP
     - /events.jsonp

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
   * - numberOfEvents
     - Number
     - The number of events.
     - True
   * - events
     - Array
     - The array of `Event object`_.
     - True
   * - servers
     - Object
     - List of `Server object`_. Keys for each `Server object`_ are server IDs which corresponds to serverId values in `Trigger object`_.
     - True

.. note:: [Condition] Always: always, True: only when result is True, False: only when result is False.

Event object
-------------
.. note:: Only the event of the triggers are returned in the current version.
.. list-table::
   :header-rows: 1

   * - Key
     - Value type
     - Brief
   * - serverId
     - Number
     - A server ID.
   * - time
     - Number
     - A time of the event.
   * - type
     - Number
     - An `Event type`_.
   * - triggerId
     - Number
     - The trigger ID.
   * - hostId
     - Number
     - A host ID.
   * - brief
     - String
     - A brief of the event.

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
   * - hosts
     - Object
     - List of `Host object`_. Keys for each `Host object`_ are host IDs which corresponds to hostId values in `Trigger object`_.

Host object
-------------
.. list-table::
   :header-rows: 1

   * - Key
     - Value type
     - Brief
   * - name
     - String
     - A hostname of the host.

Event type
-------------
.. list-table::

   * - 0
     - EVENT_TYPE_ACTIVATED
   * - 1
     - EVENT_TYPE_DEACTIVATED
   * - 2
     - EVENT_TYPE_UNKNOWN
