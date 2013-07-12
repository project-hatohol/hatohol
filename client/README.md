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

### An example to intall Django on CentOS 6.4

    # yum install python-setuptools
    # easy_install pip
    # pip install django

### An example to intall bootstrap on CentOS 6.4

    $ mkdir -p ~/tmp-bs
    $ cd ~/tmp-bs
    $ wget http://twitter.github.io/bootstrap/assets/bootstrap.zip
    $ unzip bootstrap.zip
    $ cp -ar bootstrap/* <Hatohol client directory>/static/

    At this point you can remove working dir: tmp-bs.

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


How to configure
----------------------------------------
You must configure if your prepared backend of THE VIEWER may runs on not a default port.

- edit mysite/urls.py to change the port

Ex.)

'''
-    url(r'^tunnel/(?P<path>.+)', jsonforward, kwargs={'server':'localhost:33194'})
+    url(r'^tunnel/(?P<path>.+)', jsonforward, kwargs={'server':'localhost:30080'})
'''

How to run
----------------------------------------
This is a standard Django project so you can run this on some WSGI compliant application server.

Of course you can run with runserver subcommand of Django's manage.py.

Ex.)

	$ ./manage.py runserver

If you want to serve to the outside of the host that this frontend run on, you need specifying the address.

Ex.)

	$ ./manage.py runserver 0.0.0.0:8000
