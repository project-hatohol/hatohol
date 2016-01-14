=========================
GET-User_role
=========================
 **Please refer when you Get an User Role.**

Request
=======

Path
----
.. list-table::
   :header-rows: 1

   * - URL
     - Comments
   * - /user-role
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
   * - numberOfUserRoles
     - Number
     - N/A
     - True
   * - userRoles
     - Array
     - N/A
     - True

.. note:: [Condition] Always: always, True: only when result is True, False: only when result is False.

User role object
----------------
.. list-table::
   :header-rows: 1

   * - Key
     - Value type
     - Brief
   * - userRoleId
     - Number
     - N/A
   * - name
     - String
     - N/A
   * - flags
     - Number
     - N/A
