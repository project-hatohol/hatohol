=========================
Common properties
=========================

.. _server-object:

Server object
=============

`Server object`_ represents a monitoring server. It contains a list of hosts to
monitor and some properties to distiguish the server.

.. list-table::
   :header-rows: 1

   * - Key
     - Value type
     - Brief
   * - nickname
     - String
     - An arbitrary nickname of the monitoring server.
   * - type
     - Number
     - Any of :ref:`server-type`.
   * - name
     - String
     - A hostname of the server.
   * - ipAddress
     - String
     - An IP Address of the monitoring monitoring server.
   * - baseURL
     - String
     - An URL of the monitoring server.
   * - hosts
     - Object
     - List of :ref:`host-object`. Keys for each :ref:`host-object` are
       host IDs.
   * - groups
     - Object
     - List of :ref:`group-object`. Keys for each :ref:`group-object` are
       group IDs.
   * - uuid
     - String
     - An UUID of this monitoring system plugin. It's provided only when the
       `type` is `7 (HAPI2 plugin)`.

.. _server-type:

Server type
=============

:ref:`server-type` specifies the types of the monitoring system that the monitoring
server uses. When the type is `7 (HAPI2 plugin)`, the plugin type is specified
by `uuid` in `Server object`_.

.. list-table::

   * - 4
     - HAPI JSON
   * - 7
     - HAPI2 plugin

.. _host-object:

Host object
=============

`Host object`_ represents a host which is monitored by a monitoring server.

.. list-table::
   :header-rows: 1

   * - Key
     - Value type
     - Brief
   * - name
     - String
     - A name of the host.

.. _group-object:

Group object
=============

`Group object`_ represents a group of hosts.

.. list-table::
   :header-rows: 1

   * - Key
     - Value type
     - Brief
   * - name
     - String
     - A name of the host group.
