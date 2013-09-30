Hatohol
=======

Overview
--------
Hatohol collects monitoring information from running monitoring systems
and shows their integrated data on one screen.
This feature enables to consolidate monitoring centers and the operators
even when monitored devices are being used in different places or
with different monitoring software.

![Overview](doc/misc/hatohol-overview.png)

Currently Hatohol provides the following information.

- Item

  Each monitoring target such as CPU load and free memory size.

- Trigger

  Current status of monitoring items.

- Event

  History of the status change of items. When an event detected, Hatohol can execute
  a user command (called action).


Project Hatohol
-----------------------------
Hatohol is an open source software developed and copyrighted by Project Hatohol.
Participation of any people is welcome
(e.g., bug fix, problem report, feature request/suggestion/discussion,
functional improvement, and so on) 

Supported monitoring software
-----------------------------
- ZABBIX 2.0
- Nagios3 (with NDOUtils)

Supported platforms
-----------------------------
- CentOS 6.4 (x86\_64)
- Ubuntu Server 12.04.2 LTS (64-bit)
- Ubuntu 13.04 (64-bit) [supported until the next Ubuntu release]

Basic architecture
------------------
Hatohol consists of a server and a client for Web UI. Hatohol server actually
gathers monitoring information and integrate them. It provides the integrated
data as a JSON format via HTTP (REST). A client internally communicates with
the server and creates a response page when a user accesses.

![BasicArchitecture](doc/misc/hatohol-basic-architecture.png)

This architectures makes to develop alternative clients easy. For example,
android applications, iOS applications, Win32 native client,
or Web applications with other frameworks can be added.

Screenshots
-----------
- dashboard
![dashboard](doc/misc/screenshot-dashboard.png)

- events
![events](doc/misc/screenshot-events.png)

- servers
![servers](doc/misc/screenshot-servers.png)

Other pages are now preparing.

Installation
------------
### Installation by RPM files (only for CentOS 6.4)
Install the following RPMs. (You also have to install packages that are
needed by them)

- [json-glib](https://github.com/project-hatohol/json-glib-for-distribution/blob/master/RPMS/x86_64/json-glib-0.12.6-1PH.x86_64.rpm?raw=true)
- [bootstrap](https://github.com/project-hatohol/bootstrap-for-hatohol/blob/master/RPMS/x86_64/bootstrap-for-hatohol-2.3.2-1PH.x86_64.rpm?raw=true)
- [Hatohol server 0.1-1](https://github.com/project-hatohol/hatohol-packages/blob/master/RPMS/hatohol-0.1-1.el6.x86_64.rpm?raw=true)
- [Hatohol client 0.1-1](https://github.com/project-hatohol/hatohol-packages/blob/master/RPMS/hatohol-client-0.1-1.el6.x86_64.rpm?raw=true)

The way to start Hatohol server and client is written in
the follwoing build instructions.

### Installation by building needed applications
- [Server installation](server/README.md)
- [Client (Web UI server) installation](client/README.md)

Plans
-----
We will add useful features. If you have requests or good ideas,
please tell us them. The following lists are our recent plans.

- 2013 Fall  : Chart (graph) support  
- 2013 Fall  : Efficient (on-demand and cached) item data acquisition  

### Rough ideas
- Programable and user-defined alert generation mechanism  
- Configuration of target hosts
- Communication with monitoring system with private IP address
- More monitoring software support

Mailing Lists
-------------
There are some mailing lists to discuss Hatohol.

* hatohol-users@lists.sourceforge.jp
  * Discuss anything about how to install and use Hatohol (for Japanese users).
  * How to subscribe: http://lists.sourceforge.jp/mailman/listinfo/hatohol-users
* hatohol-users@lists.sourceforge.net
  * Discuss anything about how to install and use Hatohol (English).
  * How to subscribe: https://lists.sourceforge.net/lists/listinfo/hatohol-users
* hatohol-commit@lists.sourceforge.net
  * Receive commit notifications & discuss each commit.
  * How to subscribe: https://lists.sourceforge.net/lists/listinfo/hatohol-commit
* For Hatohol developers
  * Currently no mailing list exists to discuss developing Hatohol.
  * Please use github issue instead: https://github.com/project-hatohol/hatohol/issues
