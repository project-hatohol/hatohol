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
   * - numberOfEvents
     - Number
     - The number of events.
     - T
   * - events
     - Array
     - The array of `Event object`_.
     - T

.. note:: [Condition] A: always, T: only when result is True, F: only when result is False.

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
   * - hostName
     - String
     - A host name the event is concerned with.
   * - brief
     - String
     - A brief of the event.

Event type
-------------
.. list-table::

   * - 0
     - TRIGGER_ACTIVATED
   * - 1
     - TRIGGER_DEACTIVATED

