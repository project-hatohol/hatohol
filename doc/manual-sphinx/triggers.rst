=========================
Trigger
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
     - /triggers.json
   * - JSONP
     - /triggers.jsonp

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
   * - result
     - Boolean
     - True on success. Otherwise False and the reason is shown in the
       element: message.
     - A
   * - message
     - String
     - Error message. This key is reply only when result is False.
     - F
   * - numberOfTriggers
     - Number
     - The number of triggers.
     - T
   * - triggers.
     - Array
     - The array of `Trigger object`_.
     - T

.. note::

**Condition** A: always, T: only when result is True, F: only when result is False.

Trigger object
-------------
.. list-table::
   :header-rows: 1

   * - Key
     - Value type
     - Brief
   * - status
     - Number
     - A trigger status.
       **[TODO]** The meaning of each number
   * - severity
     - Number
     - A severity of the trigger.
       **[TODO]** The meaning of each number.
   * - lastChangeTime
     - Number
     - A last change time of the trigger.
   * - serverId
     - Number
     - A server ID.
   * - hostId
     - String
     - A host ID.
   * - hostName
     - String
     - A host name the trigger is concerned with.
   * - brief
     - String
     - A brief of the trigger.
