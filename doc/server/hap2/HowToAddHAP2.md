How to Add HAP2
===============

## 1. Create your plugin!

We always write some plugin with python.

If you write plugin with python, you can use some library.

The library put on /hatohol/server/hap2/hatohol

Ex. hapcommon.py, hapib.py, rabbitmqconnector.py

And, you create your plugin's UUID.

The following command is example to create UUID by Ubuntu.

```
$ uuidgen

6d12c56b-47e6-465b-89d7-5544e6afe97d
```

The UUID is used in the next sections.

## 2. How to provide server-type.sql

When you give some paramater(Ex. IP Address, Password, etc...) to your plugin,
you can check it[https://github.com/project-hatohol/HAPI-2.0-Specification#user-content-getmonitoringserverinfomethod]

You can set the parameters that are written by the above document.

Please refer to the other sql file that put on the /hatohol/sever/data.

Ex. 90-server-type-hap2-zabbix.sql, 90-server-type-hap2-nagios-ndoutils.sql

And, you should set priority to the head of the file name.

We recommend to set '90-'.

## 3. How to provide WebUI plugin

You should create WebUI plugin for JavaScript on /hatohol/client/static/js.plugin.

The following codes are most simple plugin.


your_plugin.js
```
(function(hatohol) {
    var self = hatohol.registerPlugin("Your Plugin UUID",
                                      "Your Plugin Name", int(Plugin sort priority));
    }(hatohol));

```

## 4. How to packaging a plugin

You write your plugin information and installing file to /hatohol/hatohol.spec.in.

The following codes is example of needed.


```
%package hap2-nagios-livestatus
Summary: Nagios livestatus plugins of HAPI2.0
Group: Applications/System
Requires: hatohol-hap2-common = %{version}
```

```
%description hap2-nagios-livestatus
Nagios livestatus plugin for hatohol arm plugins version 2.0.
```

```
%files hap2-nagios-livestatus
%{_datadir}/hatohol/sql/90-server-type-hap2-nagios-livestatus.sql
%{_libexecdir}/hatohol/hap2/start-stop-hap2-nagios-livestatus.sh
%{_libexecdir}/hatohol/hap2/hatohol/hap2_nagios_livestatus.py
%{_libexecdir}/hatohol/hap2/hatohol/hap2_nagios_livestatus.pyc
%{_libexecdir}/hatohol/hap2/hatohol/hap2_nagios_livestatus.pyo
```

If we need some procedure to use your plugin, you write the command of the procedure.
```
%post hap2-nagios-livestatus
pip install python-mk-livestatus
```
