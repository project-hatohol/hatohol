Hatohol Client
==============

Hatohol Client is a frontend part of Hatohol and provides a web UI based
on Django.

## Table of Contents

- [Required software and settings](#user-content-required-software-and-settings)
- [Containing modules](#user-content-containing-modules)
- [How to setup](#user-content-how-to-setup)
	- [Install Hatohol Client](#user-content-install-hatohol-client)
	- [Install Required Libraries](#user-content-install-required-libraries)
		- [On CentOS 7](#user-content-on-centos-7)
		- [On Ubuntu 12.04 and 14.04](#user-content-on-ubuntu-1204-and-1404)
	- [Create the database](#user-content-create-the-database)
	- [Create the database and the tables](#user-content-create-the-database-and-the-tables)
- [How to run](#user-content-how-to-run)
- [Hints](#user-content-hints)
	- [How to set a Hatohol server address and the port](#user-content-how-to-set-a-hatohol-server-address-and-the-port)

## Required software and settings
- Python 2.6
- Django 1.5
- JavaScript enabled (web browser side)

## Containing modules
This application contains following foreign modules. Thanks a lot!
- jQuery 1.9.1 (http://jquery.com/)
- Bootstrap 3.1.0 (http://getbootstrap.com/)
- Stupid-table-plugin
maintained by [joequery]
(http://joequery.github.io/Stupid-Table-Plugin/)

## How to setup
> See also [this page](https://github.com/project-hatohol/website/blob/master/contents/docs/install/17.06/en/index.md)
> to setup Hatohol for CentOS with the binary packages.

### Install Hatohol Client
Hatohol Client is written as PATH FREE. So you can place it anywhere.

### Install Required Libraries
The following shows examples to install Required Packages.

#### On CentOS 7

    # yum install python-pip python-devel django gettext
    # pip install mysql-python

#### On Ubuntu 12.04 and 14.04

    $ sudo apt-get install python-pip python-dev python-django gettext
    $ sudo pip install mysql-python


### Create the database
    $ mysql -u root -p
    Enter password:(input password)

    mysql> CREATE DATABASE hatohol_client DEFAULT CHARACTER SET utf8;
    mysql> GRANT ALL PRIVILEGES ON hatohol_client.* TO hatohol@localhost IDENTIFIED BY 'hatohol';

### Create the database and the tables
You must change the current directory to "client" under the top directory.

    $ ./manage.py syncdb

## How to run
Hatohol Client is a standard Django project. So you can run it on any WSGI
compliant application server.

You must change the current directory to "client" under the top directory.

Alternatively you can run with a runserver sub-command of Django's manage.py.

	$ ./manage.py runserver

If you allow to access from the outside, you need specifying the address like

	$ ./manage.py runserver 0.0.0.0:8000

## Hints
### How to set a Hatohol server address and the port
Edit hatohol/hatoholserver.py and update the following lines.

    DEFAULT_SERVER_ADDR = 'localhost'
    DEFAULT_SERVER_PORT = 33194
