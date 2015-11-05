=========================
POST-Login
=========================
 **Please refer when you Log in.**
Request
=======

Path
----
.. list-table::
   :header-rows: 1

   * - URL
     - Comments
   * - /login
     - N/A

Parameters
----------
.. list-table::
   :header-rows: 1

   * - Parameter
     - Value
     - Comments
     - Necessity
   * - password
     - N/A
     - N/A
     - Mandatory
   * - user
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
   * - sessionId
     - String
     - Session Id.
     - True

.. note:: [Condition] Always: always, True: only when result is True, False: only when result is False.
