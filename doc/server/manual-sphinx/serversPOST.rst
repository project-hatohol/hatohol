=========================
POST Server
=========================

 **Please refer when you Make a New Monitoring Server.**

Request
=======

Path
----
.. list-table::
   :header-rows: 1

   * - URL
     - Comments
   * - /server
     - N/A


Parameters
----------
| **Zabbix** 
.. list-table::
   :header-rows: 1

   * - Parameter
     - Value
     - Comments
   * - type
     - N/A
     - N/A
   * - nickname
     - N/A
     - N/A
   * - hostName
     - N/A
     - N/A
   * - ipAddress
     - N/A
     - N/A
   * - port
     - N/A
     - N/A
   * - userName
     - N/A
     - N/A
   * - password
     - N/A
     - N/A
   * - pollingInterval
     - N/A
     - N/A
   * - retryInterval
     - N/A
     - N/A

| 
| **Nagios** 
.. list-table::
   :header-rows: 1

   * - Parameter
     - Value
     - Comments
   * - type
     - N/A
     - N/A
   * - nickname
     - N/A
     - N/A
   * - hostName
     - N/A
     - N/A
   * - ipAddress
     - N/A
     - N/A
   * - port
     - N/A
     - N/A
   * - dbName
     - N/A
     - N/A
   * - userName
     - N/A
     - N/A
   * - password
     - N/A
     - N/A
   * - pollingInterval
     - N/A
     - N/A
   * - retryInterval
     - N/A
     - N/A
   * - baseURL
     - N/A
     - N/A

| 
| **Zabbix (HAPI)**
.. list-table::
   :header-rows: 1

   * - Parameter
     - Value
     - Comments
   * - type
     - N/A
     - N/A
   * - nickname
     - N/A
     - N/A
   * - hostName
     - N/A
     - N/A
   * - ipAddress
     - N/A
     - N/A
   * - port
     - N/A
     - N/A
   * - userName
     - N/A
     - N/A
   * - password
     - N/A
     - N/A
   * - pollingInterval
     - N/A
     - N/A
   * - retryInterval
     - N/A
     - N/A
   * - passiveMode
     - N/A
     - N/A
   * - brokerUrl
     - N/A
     - N/A
   * - staticQueueAddress
     - N/A
     - N/A

| 
| **JSON (HAPI)**
.. list-table::
   :header-rows: 1

   * - Parameter
     - Value
     - Comments
   * - type
     - N/A
     - N/A
   * - nickname
     - N/A
     - N/A
   * - brokerUrl
     - N/A
     - N/A
   * - staticQueueAddress
     - N/A
     - N/A
   * - tlsCertificatePath
     - N/A
     - N/A
   * - tlsKeyPath
     - N/A
     - N/A
   * - tlsCACertificatePath
     - N/A
     - N/A
   * - tlsEnableVerify
     - N/A
     - N/A

| 
| **Ceilometer**
.. list-table::
   :header-rows: 1

   * - Parameter
     - Value
     - Comments
   * - type
     - N/A
     - N/A
   * - nickname
     - N/A
     - N/A
   * - hostName
     - N/A
     - N/A
   * - ipAddress
     - N/A
     - N/A
   * - port
     - N/A
     - N/A
   * - dbName
     - N/A
     - N/A
   * - userName
     - N/A
     - N/A
   * - password
     - N/A
     - N/A
   * - pollingInterval
     - N/A
     - N/A
   * - retryInterval
     - N/A
     - N/A
   * - passiveMode
     - N/A
     - N/A
   * - brokerUrl
     - N/A
     - N/A
   * - staticQueueAddress
     - N/A
     - N/A

| 
| **Zabbix (HAPI2) / nagios (HAPI2)**
.. list-table::
   :header-rows: 1

   * - Parameter
     - Value
     - Comments
   * - type
     - N/A
     - N/A
   * - nickname
     - N/A
     - N/A
   * - baseURL
     - N/A
     - N/A
   * - userName
     - N/A
     - N/A
   * - password
     - N/A
     - N/A
   * - pollingInterval
     - N/A
     - N/A
   * - retryInterval
     - N/A
     - N/A
   * - passiveMode
     - N/A
     - N/A
   * - brokerUrl
     - N/A
     - N/A
   * - staticQueueAddress
     - N/A
     - N/A
   * - tlsCertificatePath
     - N/A
     - N/A
   * - tlsKeyPath
     - N/A
     - N/A
   * - tlsCACertificatePath
     - N/A
     - N/A
   * - tlsEnableVerify
     - N/A
     - N/A

| 
| **Ceilometer(HAPI2)**
.. list-table::
   :header-rows: 1

   * - Parameter
     - Value
     - Comments
   * - type
     - N/A
     - N/A
   * - nickname
     - N/A
     - N/A
   * - baseURL
     - N/A
     - N/A
   * - userName
     - N/A
     - N/A
   * - password
     - N/A
     - N/A
   * - extendedInfo
     - N/A
     - N/A
   * - pollingInterval
     - N/A
     - N/A
   * - retryInterval
     - N/A
     - N/A
   * - passiveMode
     - N/A
     - N/A
   * - brokerUrl
     - N/A
     - N/A
   * - staticQueueAddress
     - N/A
     - N/A
   * - tlsCertificatePath
     - N/A
     - N/A
   * - tlsKeyPath
     - N/A
     - N/A
   * - tlsCACertificatePath
     - N/A
     - N/A
   * - tlsEnableVerify
     - N/A
     - N/A

| 
| **Fluentd(HAPI2)**
.. list-table::
   :header-rows: 1

   * - Parameter
     - Value
     - Comments
   * - type
     - N/A
     - N/A
   * - nickname
     - N/A
     - N/A
   * - passiveMode
     - N/A
     - N/A
   * - brokerUrl
     - N/A
     - N/A
   * - staticQueueAddress
     - N/A
     - N/A
   * - tlsCertificatePath
     - N/A
     - N/A
   * - tlsKeyPath
     - N/A
     - N/A
   * - tlsCACertificatePath
     - N/A
     - N/A
   * - tlsEnableVerify
     - N/A
     - N/A


Server type
-------------
.. list-table::

   * - 0
     - * Zabbix
   * - 1
     - * Nagios
   * - 2
     - * Zabbix (HAPI)
   * - 4
     - * JSON (HAPI)
   * - 6
     - * Ceilometer
   * - 7
     - * Zabbix (HAPI2)
       * Nagios (HAPI2)
       * Ceilometer (HAPI2)
       * Fluentd (HAPI2)

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

