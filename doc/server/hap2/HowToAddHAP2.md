How to Add HAP2
===============

## 1. Create your plugin!

We always write some plugin with python.

If you write plugin with python, you can use some library.

The library put on /hatohol/server/hap2/hatohol

Ex. hapcommon.py, hapib.py, rabbitmqconnector.py

And, you create your plugin UUID.

The following command is example to create on Ubuntu.

```
$ uuidgen

6d12c56b-47e6-465b-89d7-5544e6afe97d
```

The uuid is used in the next sections.

## 2. How to provide server-type.sql

You want to give some paramater to your plugin.(Ex. IP Address, Password, etc...)

If you so, you can check it[https://github.com/project-hatohol/HAPI-2.0-Specification#user-content-getmonitoringserverinfomethod]

You can set the parameter that is written by the above document.

Please refer to the other sql file that put on the /hatohol/sever/data

Ex. 90-server-type-hap2-zabbix.sql, 90-server-type-hap2-nagios-ndoutils.sql

And, you should set priority to the head of the file name.

The prioprity is used by hatohol-db-initiator.

We recommend to set '90-'.

## 3. How to provide WebUI plugin



## 4. How to packaging a plugin


