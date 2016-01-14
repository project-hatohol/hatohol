=========================
GET-User
=========================
 **Please refer when you Get an User.**

Request
=======

Path
----
.. list-table::
   :header-rows: 1

   * - URL
     - Comments
   * - /tunnel/user
     - To get about all user.
   * - /user/me
     - To get about logined user.

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
   * - numberOfUsers
     - Number
     - N/A
     - True
   * - users
     - Array
     - N/A
     - True
   * - userRoles
     - Array
     - N/A
     - True

.. note:: [Condition] Always: always, True: only when result is True, False: only when result is False.

User object
--------------
.. list-table::
   :header-rows: 1

   * - Key
     - Value type
     - Brief
   * - userId
     - Number
     - N/A
   * - name
     - String
     - N/A
   * - flags
     - Number
     - N/A

User role object
----------------
.. list-table::
   :header-rows: 1

   * - Key
     - Value type
     - Brief
   * - name
     - String
     - N/A
