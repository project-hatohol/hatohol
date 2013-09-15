=========================
Trigger
=========================

Request
=======

Path
----
.. list-table::
   :header-rows: 1

   * - URL
     - Comments
   * - /trigger
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
   * - numberOfTriggers
     - Number
     - The number of triggers.
     - True
   * - triggers
     - Array
     - The array of `Trigger object`_.
     - True
   * - servers
     - Object
     - List of `Server object`_. Keys for each `Server object`_ are server IDs which corresponds to serverId values in `Trigger object`_.
     - True

.. note:: [Condition] Always: always, True: only when result is True, False: only when result is False.

Trigger object
--------------
.. list-table::
   :header-rows: 1

   * - Key
     - Value type
     - Brief
   * - status
     - Number
     - A `Trigger status`_.
   * - severity
     - Number
     - A `Trigger severity`_.
   * - lastChangeTime
     - Number
     - A last change time of the trigger.
   * - serverId
     - Number
     - A server ID.
   * - hostId
     - Number
     - A host ID.
   * - brief
     - String
     - A brief of the trigger.

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

Trigger status
--------------
.. list-table::

   * - 0
     - TRIGGER_STATUS_OK
   * - 1
     - TRIGGER_STATUS_PROBLEM

.. _ref-trigger-severity:

Trigger severity
----------------
.. list-table::

   * - 0
     - TRIGGER_SEVERITY_INFO
   * - 1
     - TRIGGER_SEVERITY_WARN
