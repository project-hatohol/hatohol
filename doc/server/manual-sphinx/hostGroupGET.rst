=========================
GET-Host_group
=========================
 **Please refer when you Get a Host_Group.**

Request
=======

Path
----
.. list-table::
   :header-rows: 1

   * - URL
     - Comments
   * - /host
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
   * - serverId
     - N/A
     - N/A
     - Optional
     - Optional

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
   * - numberOfHostgroups
     - Number
     - N/A
     - True
   * - hostgroups
     - Array
     - N/A
     - True

.. note:: [Condition] Always: always, True: only when result is True, False: only when result is False.

Host group object
-----------------
.. list-table::
   :header-rows: 1

   * - Key
     - Value type
     - Brief
   * - id
     - Number
     - N/A
   * - serverId
     - Number
     - N/A
   * - groupId
     - String
     - N/A
   * - groupName
     - String
     - N/A
   * - hosts
     - Array
     - N/A

Host object
--------------
.. list-table::
   :header-rows: 1

   * - Value type
     - Brief
   * - String
     - N/A
