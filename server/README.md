Hatohol Server
==============

Hatohol acquires the monitoring data from multiple monitoring software,
consolidates them, serves the data with the format that is independent of
the specific monitoring software in various way.

Supported monitoring software
-----------------------------
- ZABBIX 2.0
- Nagios3 (with NDOUtils)

Supported output method
------------------------
- REST

Supported platforms
-------------------
- CentOS 6.5 (x86\_64)
- Ubuntu Server 14.04LTS (64-bit)
- Ubuntu 14.04 (64-bit) [supported until the next ubuntu release]

Required libraries
------------------
- sqlite3 >= 3.6.0 (https://www.sqlite.org/)
- MySQL
- libsoup >= 2.22 (https://live.gnome.org/LibSoup)
- json-glib >= 0.12.0 (https://live.gnome.org/JsonGlib/)
  (CentOS and EPEL do not contain json-glib.)
- libc
- librt
- libstdc++
- uuid-dev
- qpidd
- libqpidmessaging2-dev
- libqpidtypes1-dev
- libqpidcommon2-dev

### Example to install required libraries on CentOS 6.5
> See also [this page](https://github.com/project-hatohol/website/blob/master/contents/docs/install/14.06/ja/index.md)
> to setup Hatohol for CentOS with the binary packages.

First, you need to install development tools to build Hatohol and some required
packages

    # yum groupinstall "Development Tools"

You need to register a yum repository for installing qpid-cpp-client-devel packages 
by the following command

    # wget -P /etc/yum.repos.d/ http://project-hatohol.github.io/repo/hatohol.repo

You can install sqlite3, MySQL and libsoup and others by the following command:

    # yum install sqlite-devel mysql-devel libsoup-devel libuuid-devel qpid-cpp-client-devel

#### For json-glib, there are two ways to install.

##### One is to use json-glib RPM package built by Project Hatohol.

- [json-glib library] (https://github.com/project-hatohol/json-glib-for-distribution/blob/master/RPMS/x86_64/json-glib-0.12.6-1PH.x86_64.rpm?raw=true)
- [Header files (devel package)] (https://github.com/project-hatohol/json-glib-for-distribution/blob/master/RPMS/x86_64/json-glib-devel-0.12.6-1PH.x86_64.rpm?raw=true)

##### The other is to install it from the source tar ball like below.

Getting json-glib:

    $ wget http://ftp.gnome.org/pub/GNOME/sources/json-glib/0.12/json-glib-0.12.6.tar.bz2
    $ tar xvfj json-glib-0.12.6.tar.bz2

Building & installing by following commands:

    $ ./configure
    $ make all
    $ su
    # make install

### Example to install required libraries on ubuntu 14.04 

You should install these package to build Hatohol and required libraries.

- automake
- g++
- libtool
- libsoup2.4-dev
- libjson-glib-dev
- libsqlite3-dev
- libmysqlclient-dev
- mysql-server
- uuid-dev
- qpidd
- libqpidmessaging2-dev
- libqpidtypes1-dev
- libqpidcommon2-dev

installing by following commands:

    $ sudo apt-get install automake g++ libtool libsoup2.4-dev libjson-glib-dev libsqlite3-dev libmysqlclient-dev mysql-server sqlite3 uuid-dev qpidd libqpidmessaging2-dev libqpidtypes1-dev libqpidcommon2-dev

How to build Hatohol
--------------------
First, you need to install required libraries.  
Then run the following commands to install Hatohol:

    $ ./autogen.sh
    $ PKG_CONFIG_PATH=/usr/local/lib/pkgconfig ./configure
    $ make
    $ su
    # make install
    # /sbin/ldconfig

How to start
------------
(0.1) Run MySQL server for storing configuration.

(0.2) Set QPid's setting
(0.2.a) Disable authentification
Add the following line in /etc/qpid/qpidd.conf

    auth=no

(0.2.b) Allow all yours (Setting of ACL)
Example of /etc/qpid/qpidd.acl

    acl allow all all

NOTE: You have to restart qpidd after you edit /etc/qpid/qpiid.acl.

(1) Setup database of MySQL

  $ mysql -uroot -p < /usr/local/share/hatohol/sql/create-db.sql

Tips:
- If the root password of the MySQL server is not set, -p option is not needed.
- The default create-db.sql sets 'hatohol'/'hatohol' as user/password of
the DB. If you change them, copy create-db.sql to any directory,
replace them in the GRANT line of the copied file, and use it
as an input of the mysql command.
- If Hatohol server and MySQL server are executed on different machines.
You have to replace 'localhost' in the GRANT line with the machine name or
the IP address that runs Hatohol server.

For example, user/password are 'myuser'/'mypasswd' and the IP address of
Hatohol server is 192.168.10.50.

    GRANT ALL PRIVILEGES ON hatohol.* TO myuser@"192.168.10.50" IDENTIFIED BY 'mypasswd';


(2) Prepare the data base directory

Hatohol automatically creates databases for storing data cache. You have to
decide the direcory for the DBs and make it if necessary.

Ex.) `mkdir /var/lib/hatohol`

(3) start hatohol process

    $ HATOHOL_DB_DIR=<directory preapred in step (2)> hatohol --config-db-server <Config DB Server:[port]>

Ex.) `$ HATOHOL_DB_DIR=/var/lib/hatohol hatohol --config-db-server localhost`

Tips:

* From the 2nd time, you can start only with the above step (3).
* If you omit the environment variable 'HATOHOL_DB_DIR', the databases are
created in /tmp.
* When the process successfully starts, hatohol reply the HTML message
including HATOHOL version on http://[hostname]:[RestPort]/hello.html.
  Ex.) `http://localhost:33194/hello.html`

How to stop
-----------
Send one of the following signals to hatohol process.

- SIGTERM
- SIGINT (This signal is also sent by Ctrl-C)
- SIGHUP
- SIGUSR1

Ex.)

    $ pkill hatohol

How to set the port number of REST service
------------------------------------------
(1) If the command line option --face-rest-port is specified, it is used.

Ex.)

    $ hatohol --face-rest-port 3000

(2) Otherwise if the port number in the configuration DB is not 0, it is used.

(3) Otherwise the default port: 33194 is used.

Trouble Shooting
----------------
### Hatohol shows "Failed to get" and doesn't acquire information
- Check if your servers are correctly registered to Hatohol's database.
You can verify them by checking the result from http://localhost:33194/servers.json.
(In case you start Hatohol with default port number) For example, if value of "servers"
is empty, no servers are registered in the database. If you can see other unexpected
results, check it too.

Tips to configure Nagios NDOUtils
---------------------------------
In the following steps, it is asummed that the nagios server,
ndoutils (ndo2db), and DB (MySQL) are on the same computer.

(1) Install an Nagios NDOUtils package

(2) Edit /etc/nagios/nagios.cfg to use an ndoutils broker module
The line like below is needed.

    broker_module=/usr/lib/ndoutils/ndomod-mysql-3x.o config_file=/etc/nagios3/ndomod.cfg

(3) Edit ndo2db.cfg to set database information.
The example of major target parameters are

    db_name=ndoutils
    db_user=ndoutils
    db_pass=admin

(4) Make a database and set the appropriate permission to it

Ex.)

    # mysql
    mysql> CREATE DATABASE ndoutils;
    mysql> GRANT all on ndoutils.* TO ndoutils@localhost IDENTIFIED BY 'admin';

(5) Make tables. The command is prepared. The following is for ubuntu 13.04

    # cd /usr/share/dbconfig-common/data/ndoutils-mysql/install
    # mysql -D ndoutils < mysql

(6) start ndo2db and nagios3

In ubuntu 13.04, edit of ENABLE_NDOUTILS variable in /etc/default/ndoutils
  is also needed.

    ENABLE_NDOUTILS=1

See also [this install document](../doc/misc/nagios-setup-cent6.md) if you use CentOS.

API Reference Manual for REST service
-------------------------------------
The API reference manual can be created as below.

    $ cd <Top directory of the Hatohol source repository>
    $ cd doc/server/manual-sphinx
    $ make html

Then you can see the manual at doc/manual-sphinx/_build/html/index.html
with a web browser.

Tips:

* You have to install SPHINX package for making the html files.
When you use Ubuntu 13.04, 'python3-sphinx' package is available.

