=========================
GET-History
=========================
 **Please refer when you Get a History.**
Request
=======

Path
----
.. list-table::
   :header-rows: 1

   * - URL
     - Comments
   * - /history
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
   * - beginTime
     - The time of the beginning by UNIX Time.
     - N/A
     - Optional
     - Optional
   * - endTime
     - The time of the end by UNIX Time.
     - N/A
     - Optional
     - Optional
   * - hostId
     - N/A
     - N/A
     - Optional
     - Optional
   * - itemId
     - N/A
     - N/A
     - Mandatory
     - Mandatory
   * - serverId
     - N/A
     - N/A
     - Mandatory
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
   * - optionMessages
     - String
     - N/A
     - False
   * - history
     - Array
     - N/A
     - True

.. note:: [Condition] Always: always, True: only when result is True, False: only when result is False.

History object
--------------
.. list-table::
   :header-rows: 1

   * - Key
     - Value type
     - Brief
   * - value
     - String
     - The value of the data.
   * - clock
     - Number
     - The time of the data by UNIX Time.
   * - ns
     - Number
     - N/A

