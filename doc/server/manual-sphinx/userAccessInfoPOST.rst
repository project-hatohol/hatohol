=========================
POST-User_access_info
=========================
 **Please refer when you Make a New User Access Info.**

Request
=======

Path
----
.. list-table::
   :header-rows: 1

   * - URL
     - Comments
   * - /user/<userId>/access-info
     - N/A

Parameters
----------
.. list-table::
   :header-rows: 1

   * - Parameter
     - Value
     - Comments
   * - hostgroupId
     - N/A
     - If you select all host groups, the value must be `*`.
   * - serverId
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
     - N/A
     - True

.. note:: [Condition] Always: always, True: only when result is True, False: only when result is False.

