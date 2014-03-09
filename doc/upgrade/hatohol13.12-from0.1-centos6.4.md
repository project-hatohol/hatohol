How to upgrade Hatohol on CentOS 6.4 (x86_64) from version 0.1 to version 13.12 with RPM files
=====================================================================

Installation of Additional packages.
-----------------------------------
### Django
Project Hatohol provides RPM file of Django.

Then you have to uninstall Django that installed by pip.

Uninstall the Django as

    # pip uninstall django

- https://github.com/project-hatohol/Django-for-distribution/raw/master/dist/Django-1.5.3-1.noarch.rpm

Install the package as

    # rpm -Uhv Django-1.5.3-1.noarch.rpm

### Additional dependent packages
Additional packages are required for Hatohol13.12, then you have to insatall packages as below.
- libuuid
- MySQL-python
- mod_wsgi
- httpd

Install the package as

    # yum install libuuid MySQL-python mod_wsgi httpd

How to update Hatohol13.12
-------------------------------
### Stop of Hatohol Server
In order to update, stop the Hatohol Server.

Execute command as below.

    # service hatohol stop

### Hatohol Server
Download the RPM from the following URL.

- https://github.com/project-hatohol/hatohol-packages/raw/master/RPMS/13.12/hatohol-13.12-1.el6.x86_64.rpm

### Hatohol Client
Download the RPM from the following URL.

- https://github.com/project-hatohol/hatohol-packages/raw/master/RPMS/13.12/hatohol-client-13.12-1.el6.x86_64.rpm

### Update of Hatohol Server and Hatohol Client
In order to update, execute command as below.

    # rpm -Uhv hatohol-13.12-1.el6.x86_64.rpm hatohol-client-13.12-1.el6.x86_64.rpm

Re-setup
---------------
### Re-specification of Hatohol cache DB.
In Hatohol13.12, the description part of environment variable'HATOHOL_DB_DIR' which specifies Hatohol chache DB is changed.

You have to prepare a directory for Hatohol cache DB. Here we use '/var/lib/hatohol' as an example.

Make the directory if needed.

    # mkdir /var/lib/hatohol

For using the above directory, you have to make '/etc/hatohol/initrc'.    
And write environmental variables 'HATOHOL_DB_DIR' as below.

    export HATOHOL_DB_DIR=/var/lib/hatohol

### Re-initialization of Hatohol DB

In Hatohol13.12, The function of user authentication was newly added.
Therefore, it is necessary to add an initial user's setup.
Moreover, the statement of server ID became unnecessary which is needed in Hatohol0.1 at the time of server registration.  

Copy the configuration template file to an arbitrary directory.

    # cp /usr/share/hatohol/hatohol-config.dat.example ~/hatohol-config.dat

Add the information of ZABBIX server and nagios server in the copied configuration file.
Some rules and examples are in the file as comments.

> ** Note **
> Since the statement of ID became unnecessary, be careful for there to be no failure to erase.

Reflect the configuration to the DB as

    # hatohol-config-db-creator hatohol-config.dat

### Setup of Hatohol Client

It is necessary to prepare DB for Hatohol Client in Hatohol13.12. 
Perform the following procedures.

- Preparation of DB for Hatohol Client.

After you login to MySQL, execute commands as below.

    > CREATE DATABASE hatohol_client;
    > GRANT ALL PRIVILEGES ON hatohol_client.* TO hatohol@localhost IDENTIFIED BY 'hatohol';

- Add the table in Hatohol Client DB.

Execute command as below.

    # /usr/libexec/hatohol/client/manage.py syncdb

### Start of Hatohol Server

    # service hatohol start


When Hatohol server successfully starts, you can see log messages such as the following in /var/log/messages.

    Oct  8 09:46:58 localhost hatohol[3038]: [INFO] <DBClientConfig.cc:336> Configuration DB Server: localhost, port: (default), DB: hatohol, User: hatohol, use password: yes
    Oct  8 09:46:58 localhost hatohol[3038]: [INFO] <main.cc:165> started hatohol server: ver. 13.12
    Oct  8 09:46:58 localhost hatohol[3038]: [INFO] <FaceRest.cc:121> started face-rest, port: 33194
    Oct  8 09:46:59 localhost hatohol[3038]: [INFO] <ArmZabbixAPI.cc:925> started: ArmZabbixAPI (server: testZbxSv1)
    Oct  8 09:47:01 localhost hatohol[3038]: [INFO] <ArmZabbixAPI.cc:925> started: ArmZabbixAPI (server: testZbxSv2)

> ** TROUBLE SHOOT ** Currenty, Hatohol Server generates all status logs into syslog as USER.INFO. By default settings, USER.INFO is routed to /var/log/messages in CentOS 6.

### View of Hatohol information
For example, if the Hatohol client runs on computer: 192.168.1.1,
Open the following URL from your Browser.

- http://192.168.1.1/viewer/

> ** Note **
> Currently the above pages have been checked with Google Chrome and Firefox.
> When using Internet Explorer, display layouts may collapse depending on the version of use.
