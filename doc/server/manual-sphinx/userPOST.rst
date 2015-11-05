=========================
POST-User
=========================
 **Please refer when you Make a new User.**

Request
=======

Path
----
.. list-table::
   :header-rows: 1

   * - URL
     - Comments
   * - /user
     - N/A

Parameters
----------
.. list-table::
   :header-rows: 1

   * - Parameter
     - Value
     - Comments
   * - flags
     - N/A
     - N/A
   * - name
     - N/A
     - User name.
   * - password
     - N/A
     - User password.

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

