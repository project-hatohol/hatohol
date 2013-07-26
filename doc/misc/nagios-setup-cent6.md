How to setup Naigos and NDOUtils on REHL6 or the compatible OSs with 3rd party RPMs
-----------------------------------------------------------------------------------

    WARNING: The instructions in this document are not fully confirmed.

(0) Get the RPMs from the following.

Nagios

- http://dl.fedoraproject.org/pub/epel/6/x86_64/nagios-common-3.5.0-1.el6.x86_64.rpm
- http://dl.fedoraproject.org/pub/epel/6/x86_64/nagios-3.5.0-1.el6.x86_64.rpm

Nagios (plugins)

- http://dl.fedoraproject.org/pub/epel/6/x86_64/nagios-plugins-1.4.16-5.el6.x86_64.rpm
- http://dl.fedoraproject.org/pub/epel/6/x86_64/nagios-plugins-load-1.4.16-5.el6.x86_64.rpm
- http://dl.fedoraproject.org/pub/epel/6/x86_64/nagios-plugins-http-1.4.16-5.el6.x86_64.rpm
- http://dl.fedoraproject.org/pub/epel/6/x86_64/nagios-plugins-ping-1.4.16-5.el6.x86_64.rpm
- http://dl.fedoraproject.org/pub/epel/6/x86_64/nagios-plugins-swap-1.4.16-5.el6.x86_64.rpm
- http://dl.fedoraproject.org/pub/epel/6/x86_64/nagios-plugins-users-1.4.16-5.el6.x86_64.rpm
- http://dl.fedoraproject.org/pub/epel/6/x86_64/nagios-plugins-ssh-1.4.16-5.el6.x86_64.rpm
- http://dl.fedoraproject.org/pub/epel/6/x86_64/nagios-plugins-disk-1.4.16-5.el6.x86_64.rpm
- http://dl.fedoraproject.org/pub/epel/6/x86_64/nagios-plugins-procs-1.4.16-5.el6.x86_64.rpm

or, if you want all plugins

- http://dl.fedoraproject.org/pub/epel/6/x86_64/nagios-plugins-all-1.4.16-5.el6.x86_64.rpm

NDOUtils

- http://dl.fedoraproject.org/pub/epel/6/x86_64/ndoutils-1.4-0.7.b9.el6.x86_64.rpm
- http://dl.fedoraproject.org/pub/epel/6/x86_64/ndoutils-mysql-1.4-0.7.b9.el6.x86_64.rpm

(1) Install the above RPMs.
(2) Edit /etc/nagios/nagios.cfg to use an ndoutils broker module
The line like below is needed.

    broker_module=/usr/lib64/nagios/brokers/ndomod.so config_file=/etc/nagios3/ndomod.cfg

(3) Edit /etc/nagios/ndo2db.cfg to set database information.
The example of major target parameters are

    db_name=ndoutils
    db_user=ndoutils
    db_pass=admin

(4) Make a database and set the appropriate permission to it

Ex.)

    # mysql -u root
    Or, the MySQL server has the root password,
    # mysql -u root -p

    mysql> CREATE DATABASE ndoutils;
    mysql> GRANT all on ndoutils.* TO ndoutils@localhost IDENTIFIED BY 'admin';
    mysql> FLUSH PRIVILEGES;

(5) Make tables. The command is prepared. The following is for ubuntu 13.04

    # cd /usr/share/doc/ndoutils-mysql-1.4/db
    # mysql -D ndoutils < mysql.sql
    Or, the MySQL server has the root password,
    # mysql -u root -p -D ndoutils < mysql.sql

(6) Start ndo2db and nagios3

    # service ndo2db start
    # service nagios start

(7) Make a password for the administrator

    # htpasswd -c /etc/nagios/passwd nagiosadmin

(8) access Nagios Web interface
When Nagios successfully starts, you can access Nagios's web intarface.

- URL: http://\<server address\>/nagios
- User: nagiosadmin


