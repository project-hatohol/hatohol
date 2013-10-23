Hatohol Client
==============

This is a frontend part of the Hatohol.
This communicates Hatohol server.
This provides web UI and shows data returned from Hatohol Server.


Required environments
----------------------------------------
- Python 2.6
- Django 1.5
- Bootstrap (from twitter)
- JavaScript enabled (web browser side)

This is a Django application.
This application does not use models and DBs.
This repository does not contain a copy of Bootstrap.

Containing modules
----------------------------------------
This application contains following foreign modules.
Thanks a lot!

- jQuery

- Stupid-table-plugin
maintained by [joequery]
(http://joequery.github.io/Stupid-Table-Plugin/)


How to setup
----------------------------------------
This Django project was written as PATH FREE so you can place this anyware without configuring about the project path.

- just copy the project directory onto where you want
- get Bootstrap then put them onto static/js or static/css

## An example to install Django
### On CentOS 6.4

    # yum install python-setuptools
    # easy_install pip
    # pip install django

### On Ubuntu 12.04

    # sudo apt-get install python-pip
    # pip install django

### On Ubuntu 13.04

    # sudo apt-get install libapache2-mod-wsgi python-django

> ** Note ** The above command installs django 1.4. If you want to install django 1.5, you do 'pip install django'.

## An example to intall bootstrap

    $ mkdir -p ~/tmp-bs
    $ cd ~/tmp-bs
    $ wget http://getbootstrap.com/2.3.2/assets/bootstrap.zip
    $ unzip bootstrap.zip
    $ cp -ar bootstrap/* <Hatohol client directory>/static/

    At this point you can remove working dir: tmp-bs.

If you use CentOS6.4, you can download the package from https://github.com/project-hatohol/bootstrap-for-hatohol


How to configure
----------------------------------------
You must configure if your prepared Hatohol server may runs on not a default port.

- edit hatohol/urls.py to change the port

Ex.)

	- url(r'^tunnel/(?P<path>.+)', jsonforward, kwargs={'server':'localhost:33194'})
	+ url(r'^tunnel/(?P<path>.+)', jsonforward, kwargs={'server':'localhost:30080'})

How to run
----------------------------------------
This is a standard Django project so you can run this on some WSGI compliant application server.

Of course you can run with runserver subcommand of Django's manage.py.

Ex.)

	$ ./manage.py runserver

If you want to serve to the outside of the host that this frontend run on, you need specifying the address.

Ex.)

	$ ./manage.py runserver 0.0.0.0:8000
