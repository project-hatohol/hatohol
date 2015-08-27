=========================
DELETE-Servers
=========================
 **Please refer when you Delete an Monitoring Server.**
Request
=======

Path
----
.. list-table::
   :header-rows: 1

   * - URL
     - Comments
   * - /server/<serverId>
     - N/A


Parameters
----------
.. list-table::
   :header-rows: 1

   * - Parameter
     - Value
     - Comments
   * - N/A
     - N/A
     - N/A

Server type
-------------
.. list-table::

   * - 0
     - Zabbix
   * - 1
     - Nagios
   * - 2
     - Zabbix (HAPI)
   * - 4
     - JSON (HAPI)
   * - 6
     - Ceilometer
   * - 7
     - Zabbix (HAPI2)
       Nagios (HAPI2)
       Ceilometer (HAPI2)
       Fluentd (HAPI2)

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
     - Server id.
     - True

.. note:: [Condition] Always: always, True: only when result is True, False: only when result is False.

