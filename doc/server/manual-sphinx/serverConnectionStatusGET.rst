=========================
GET-Server_Connection_Status
=========================

 **Please refer when you Get a Server Connection Status.**

Request
=======

Path
----
.. list-table::
   :header-rows: 1

   * - URL
     - Comments
   * - /server-conn-stat
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
   * - serverConnStat
     - Array
     - The array of `ServerConnStat object`_.
     - True

.. note:: [Condition] Always: always, True: only when result is True, False: only when result is False.

ServerConnStat object
-------------
.. list-table::
   :header-rows: 1

   * - Key
     - Value type
     - Brief
   * - running
     - Number
     - N/A
   * - status
     - Number
     - N/A
   * - statUpdateTime
     - String
     - N/A
   * - lastSuccessTime
     - String
     - N/A
   * - lastFailureTime
     - String
     - N/A
   * - failureComment
     - String
     - N/A
   * - numUpdate
     - Number
     - N/A
   * - numFailure
     - Number
     - N/A
