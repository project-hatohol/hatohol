Hatohol Server
==============

Hatohol acquires the monitoring data from multiple monitoring software,
consolidates them, serves the data with the format that is independent of
the specific monitoring software in various way.

Supported monitoring software
-----------------------------
- ZABBIX 2.0

Supported output method
------------------------
- REST

Supported platforms
-------------------
- CentOS 6.4 (x86_64)
- Ubuntu Server 12.04.2 LTS (64-bit)
- Ubuntu 13.04 (64-bit) [supported until the next ubuntu release]

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

### Example to install required libraries on CentOS 6.4
First, you need to install development tools to build Hatohol and some required
packages

    # yum groupinstall "Development Tools"

You can install sqlite3, MySQL and libsoup by following command:

    # yum install sqlite-devel mysql-devel libsoup-devel

For json-glib, there are two ways to install.

    * One is to use json-glib RPM package built by Project Hatohol.

        - https://github.com/project-hatohol/json-glib-for-distribution/blob/master/RPMS/x86\_64/json-glib-0.12.6-1PH.x86\_64.rpm?raw=true
        - https://github.com/project-hatohol/json-glib-for-distribution/blob/master/RPMS/x86\_64/json-glib-devel-0.12.6-1PH.x86\_64.rpm?raw=true

    * The other is to install it from the source tar ball like below.

        Getting json-glib:

            $ wget http://ftp.gnome.org/pub/GNOME/sources/json-glib/0.12/json-glib-0.12.6.tar.bz2
            $ tar xvfj json-glib-0.12.6.tar.bz2

        Building & installing by following commands:

            $ ./configure
            $ make all
            $ su
            # make install

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
(1) Make configuration database

[1-1] Prepare configuraiont data text file as a source of binary database.

Tips:

* There is a template file in <install path>/share/hatohol/hatohol-config.dat.example.   
  Ex.) `cp /usr/local/share/hatohol/hatohol-config.dat.example hatohol-config.dat`

[1-2] Add server information to hatohol-config.dat

[1-3] Create the binary database

    $ hatohol-config-db-creator hatohol-config.dat hatohol-config.db

where 'hatohol-config.db' is the binary database and its name is arbitrary.

(2) Prepare the data base directory 

Hatohol automatically creates databases for storing its data. You have to 
decide the direcory for the DBs and make it if necessary.

Ex.) `mkdir /var/lib/hatohol`

(3) start hatohol process

    $ HATOHOL_DB_DIR=<directory preapred in step (2)> hatohol --config-db <path>/hatohol-config.db

Ex.) `$ HATOHOL_DB_DIR=/var/lib/hatohol hatohol --config-db /var/lib/hatohol-config.db`

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
- Do not change or remove user "Admin" and its password (zabbix)
Currently Hatohol tries to login as "Admin" to get information from zabbix server.
- Check if your servers are correctly registered to Hatohol's database.
You can verify them by checking the result from http://localhost:33194/servers.json.
(In case you start Hatohol with default port number) For example, if value of "servers"
is empty, no servers are registered in the database. If you can see other unexpected
results, check it too.
Even if no servers are registered in the database or you specify non existent sqlite
database with --config-db parameter, Hatohol starts without any error messages. If the
specified database doesn't exist, Hatohol automatically create it.

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

In ubuntsu 13.04, edit of ENABLE_NDOUTILS variable in /etc/default/ndoutils
  is also needed.

    ENABLE_NDOUTILS=1

API Reference Manual for REST service
-------------------------------------
The API reference manual can be created as below.

    $ cd doc/manual-sphinx
    $ make html

Then you can see the manual at doc/manual-sphinx/_build/html/index.html
with a web browser.

Tips:

* You have to install SPHINX package for making the html files.
When you use Ubuntu 13.04, 'python3-sphinx' package is available.

