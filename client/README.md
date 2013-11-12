Hatohol Client
==============

Hatohol Clinet is a frontend part of Hatohol and provides a web UI based
on Django.

Required software and settings
----------------------------------------
- Python 2.6
- Django 1.5
- Bootstrap (from twitter)
- JavaScript enabled (web browser side)

Containing modules
----------------------------------------
This application contains following foreign modules. Thanks a lot!
- jQuery
- Stupid-table-plugin
maintained by [joequery]
(http://joequery.github.io/Stupid-Table-Plugin/)

How to setup
----------------------------------------
> See also [this page](../doc/install/hatohol0.1-centos6.4.md)
> to setup Hatohol for CentOS with the binary packages.

### Install Hatohol Client
Hatohol Client is written as PATH FREE. So you can place it anywhere.

### Install Bootstrap
Get a Bootstrap tar ball (http://getbootstrap.com/) and extract them
onto static/js and static/css.

### Install Django 1.5
The following shows examples to install Django.

#### On CentOS 6.4

    # yum install python-setuptools
    # easy_install pip
    # pip install django==1.5.4
    # pip install mysql-python

#### On Ubuntu 12.04 and 13.10

    # sudo apt-get install python-pip
    # pip install django==1.5.4
    # sudo apt-get install python-dev
    # pip install mysql-python


### Create the database

    > CREATE DATABASE hatohol_client;
    > GRANT ALL PRIVILEGES ON hatohol_client.* TO hatohol@localhost IDENTIFIED BY 'hatohol';

### Create the database and the tables

    $ ./manage.py syncdb

How to run
----------------------------------------
Hatohol Client is a standard Django project. So you can run it on any WSGI
compliant application server.

Alternatively you can run with a runserver sub-command of Django's manage.py.

	$ ./manage.py runserver

If you allow to access from the outside, you need specifying the address like

	$ ./manage.py runserver 0.0.0.0:8000

Hints
-----
### How to set a Hatohol server address and the port
Edit hatohol/hatoholserver.py and update the following lines.

    DEFAULT_SERVER_ADDR = 'localhost'
    DEFAULT_SERVER_PORT = 33194
