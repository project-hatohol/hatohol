Hatohol Server
==============

Hatohol acquires the monitoring data from multiple monitoring software,
consolidates them, serves the data with the format that is independent of
the specific monitoring software in various way.

## Table of Contents

- [Supported monitoring software](#user-content-supported-monitoring-software)
- [Supported incident tracking software](#user-content-supported-incident-tracking-software)
- [Supported output method](#user-content-supported-output-method)
- [Supported platforms](#user-content-supported-platforms)
- [Required libraries](#user-content-required-libraries)
	- [Example to install required libraries on CentOS 6.5](#user-content-example-to-install-required-libraries-on-centos-65)
		- [For json-glib, there are two ways to install.](#user-content-for-json-glib-there-are-two-ways-to-install)
			- [One is to use json-glib RPM package built by Project Hatohol.](#user-content-one-is-to-use-json-glib-rpm-package-built-by-project-hatohol)
			- [The other is to install it from the source tar ball like below.](#user-content-the-other-is-to-install-it-from-the-source-tar-ball-like-below)
	- [Example to install required libraries on ubuntu 14.04](#user-content-example-to-install-required-libraries-on-ubuntu-1404)
- [How to build Hatohol](#user-content-how-to-build-hatohol)
- [How to start](#user-content-how-to-start)
- [How to stop](#user-content-how-to-stop)
- [How to set the port number of REST service](#user-content-how-to-set-the-port-number-of-rest-service)
- [Trouble Shooting](#user-content-trouble-shooting)
	- [Hatohol shows "Failed to get" and doesn't acquire information](#user-content-hatohol-shows-failed-to-get-and-doesnt-acquire-information)
- [Tips to configure Nagios NDOUtils](#user-content-tips-to-configure-nagios-ndoutils)
- [API Reference Manual for REST service](#user-content-api-reference-manual-for-rest-service)

## Supported monitoring software
- Zabbix 2.0
- Zabbix 2.2
- Zabbix 2.4
- Nagios3 (with NDOUtils)
- Nagios4 (with NDOUtils)
- Ceilometer (OpenStack)
- Fluentd

## Supported incident tracking software
- Redmine

## Supported output method
- REST

## Supported platforms
- CentOS 6.5 (x86\_64)
- Ubuntu Server 14.04LTS (64-bit)
- Ubuntu 14.04 (64-bit) [supported until the next ubuntu release]

## Required libraries
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

    # wget -P /etc/yum.repos.d/ http://project-hatohol.github.io/repo/hatohol-el6.repo

You can add a new repository the following command.

    # yum install epel-release

You can install sqlite3, MySQL and libsoup and others by the following command:

    # yum install sqlite-devel mysql-devel libsoup-devel libuuid-devel qpid-cpp-client-devel librabbitmq-devel

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

In addition, you need to install following libraries if you want to enable HAPI
(Hatohol Arm Plugin Interface) 2.0.

- librabbitmq-dev
- rabbitmq-server

You can install them by the following command:

    $ sudo apt-get install librabbitmq-dev rabbitmq-server

## How to build Hatohol
First, you need to install Django.

    $ pip3 install django

Second, you need to install required libraries.
Then run the following commands to install Hatohol:

    $ ./autogen.sh
    $ PKG_CONFIG_PATH=/usr/local/lib/pkgconfig ./configure
    $ make
    $ su
    # make install
    # /sbin/ldconfig

## How to start
(0.1) Run MySQL server for storing configuration.
Type the following comannds to run MySQL server.

    # mysqld

(0.2) Set QPid's setting
(0.2.a) Disable authentification
Add the following line in /etc/qpid/qpidd.conf
In line 33, change #auto=yes to

    auth=no

(0.2.b) Allow all yours (Setting of ACL)
Example of /etc/qpid/qpidd.acl
Change line 33.

    acl allow all all

NOTE: You have to restart qpidd after you edit /etc/qpid/qpidd.acl.

(0.3) Set RabbitMQ's setting

TBD

(0.4) Create a directory to save a PID file.

    # mkdir -p /usr/local/var/run

(1) Setup database of MySQL

To setup database used by hatohol server, you need to execute helper command `hatohol-db-initiator`.

Before you execute this helper command, you can edit mysql section in `${prefix}/etc/hatohol/hatohol.conf` which is used for database settings (username, database name and password).
Then, run this helper command as follows:

    $ hatohol-db-initiator --db_user DBUSER --db_password DBPASSWORD

You must specify DBUSER and PASSWORD whose MySQL administrator user, respectively.

Example:

    $ hatohol-db-initiator --db_user root --db_password rootpass

Note: Since 15.03, hatohol-db-initiator doesn't require command line argument after hatohol database is created.
`db_name`, `db_user` and `db_password` are read from `hatohol.conf` by default.
For example, `hatohol.conf` is placed at `/etc/hatohol/hatohol.conf`.

Tips:
- If the root password of the MySQL server is not set, just pass '' for `--db_password`.
- You can change password of the created DB by --hatohol-db-user and --hatohol-db-password options.
- You can change database, username and password by editing mysql section in `hatohol.conf`.
- If Hatohol server and MySQL server are executed on different machines, you have to input GRANT statement manually with the mysql command line tool.

For example, user/password are 'myuser'/'mypasswd' and the IP address of
Hatohol server is 192.168.10.50.

    mysql> GRANT ALL PRIVILEGES ON hatohol.* TO myuser@"192.168.10.50" IDENTIFIED BY 'mypasswd';


NOTE: If you meet the folloing error, there're two ways to solve it.

    ImportError: No module named argparse

This error happens on the system that doesn't have argparse Python package such as default state of CentOS 6.5.

(1.1.a) EPEL

    # rpm -ivh http://dl.fedoraproject.org/pub/epel/6/x86_64/epel-release-6-8.noarch.rpm
    # yum install python-argparse

(1.1.b) easy_install

    # yum install python-setuptools
    # easy_install argparse

NOTE: If you meet the folloing error, set LD_LIBRARY_PATH like the way in the section (1.2) or configure /etc/ld.so.conf.

    OSError: libhatohol.so: cannot open shared object file: No such file or directory

(1.2) Example to set LD_LIBRARY_PATH

    $  export LD_LIBRARY_PATH=/usr/local/lib:$LD_LIBRARY_PATH

(2) start hatohol process

    $  hatohol [--db-server <Config DB Server:[port]>]

Ex.) `$ hatohol --db-server localhost`

Tips:

* From the 2nd time, you can start only with the above step (2).
* When the process successfully starts, hatohol reply the HTML message
including HATOHOL version on http://[hostname]:[RestPort]/hello.html.
  Ex.) `http://localhost:33194/hello.html`

## How to stop
Send one of the following signals to hatohol process.

- SIGTERM
- SIGINT (This signal is also sent by Ctrl-C)
- SIGHUP
- SIGUSR1

Ex.)

    $ pkill hatohol

## How to set the port number of REST service
(1) If the command line option --face-rest-port is specified, it is used.

Ex.)

    $ hatohol --face-rest-port 3000

(2) Otherwise if the port number in the configuration DB is not 0, it is used.

(3) Otherwise the default port: 33194 is used.

## Trouble Shooting
### Hatohol shows "Failed to get" and doesn't acquire information
- Check if your servers are correctly registered to Hatohol's database.
You can verify them by checking the result from http://localhost:33194/servers.json.
(In case you start Hatohol with default port number) For example, if value of "servers"
is empty, no servers are registered in the database. If you can see other unexpected
results, check it too.

## Tips to configure Nagios NDOUtils
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

## API Reference Manual for REST service
The API reference manual can be created as below.

    $ cd <Top directory of the Hatohol source repository>
    $ cd doc/server/manual-sphinx
    $ make html

Then you can see the manual at doc/manual-sphinx/_build/html/index.html
with a web browser.

Tips:

* You have to install SPHINX package for making the html files.
When you use Ubuntu 13.04, 'python3-sphinx' package is available.
