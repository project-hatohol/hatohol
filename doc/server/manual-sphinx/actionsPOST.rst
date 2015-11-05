=========================
POST-Action
=========================
 **Please refer hen you Make a New Action.**
Request
=======

Path
----
.. list-table::
   :header-rows: 1

   * - URL
     - Comments
   * - /action
     - N/A

Parameters
----------
.. list-table::
   :header-rows: 1

   * - Parameter
     - Value
     - Comments
   * - command
     - N/A
     - N/A
   * - hostId
     - N/A
     - N/A
   * - hostgroupId
     - N/A
     - N/A
   * - serverId
     - N/A
     - N/A
   * - timeout
     - N/A
     - N/A
   * - triggerId
     - N/A
     - N/A
   * - triggerSeverity
     - N/A
     - N/A
   * - triggerSeverityCompType
     - N/A
     - N/A
   * - triggerStatus
     - N/A
     - N/A
   * - type
     - N/A
     - N/A
   * - workingDirectory
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
     - Number
     - N/A
     - Always
   * - errorMessage
     - String
     - N/A
     - False
   * - id
     - Number
     - Action Id.
     - True

.. note:: [Condition] Always: always, True: only when result is True, False: only when result is False.
