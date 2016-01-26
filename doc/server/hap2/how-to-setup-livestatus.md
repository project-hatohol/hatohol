# How to setup Livestatus API
-----------------------------------------------------------------------------------

You should install livestatus module if you use hap2_nagios_livestatus.py plugin.

This document describes how to install livestatus module on CentOS 7 and Ubuntu 14.04 LTS.

This document assumes that Nagios is already installed.


## (1) Install the livestatus module package

### CentOS 7

    # yum install epel-release
    # yum install check-mk-livestatus 

### Ubuntu 14.04 LTS

    # apt-get install check-mk-livestatus

## (2) Check livestauts library path

Execute the following command on each OS to see the path of "livestatus.o".

The path obtained above should be replaced with LIVESTATUS_LIB_FILE_PATH in the next section.

### CentOS 7

    # rpm -ql check-mk-livestatus

### Ubuntu

    # dpkg -L check-mk-livestatus

## (3) Edit /etc/nagios/nagios.cfg to use an livestatus broker module

Add the following line to /etc/nagios/nagios.cfg.

Livestatus module makes an AF UNIX socket file.

ANY_SOCKET_FILE_PATH designates this socket file path.

```
broker_module=LIVESTATUS_LIB_FILE_PATH config_file=ANY_SOCKET_FILE_PATH
```

## (3') Setup xinetd

If you want to activate hap2_nagios_livestatus.py outside the nagios local machine,

you should setup the xinetd. This section describes how to do it.

### Installation on CentOS 7

    # yum install xinetd

### Installation on Ubuntu 14.04 LTS

    # apt-get install xinetd

### Put livestatus configuration file

An example configuration file is as follows:

```
service livestatus
{
	type        = UNLISTED
	port        = 6557
	socket_type = stream
	protocol    = tcp
	wait        = no
	cps         = 100 3
	instances   = 500
	per_source  = 250
	flags       = NODELAY
	user        = nagios
	server      = /usr/bin/unixcat
	server_args = ANY_SOCKET_FILE_PATH
	disable     = no
#	only_from   = 127.0.0.1
}
```

Put this file to /etc/xinetd.d/livestatus.

### Start xinetd

    # service xinetd start


## (4) Start nagios

    # service nagios start

## (4') Test

You can confirm whether livestatus module is running correctly by the following command.

    # echo 'GET contacts' | unixcat ANY_FILE_PATH

If you see the following output, livestatus module setting is completed.
The output depends on the your Nagios environment.

```
address1;address2;address3;address4;address5;address6;alias;can_submit_commands;custom_variable_names;custom_variable_values;custom_variables;email;host_notification_period;host_notifications_enabled;in_host_notification_period;in_service_notification_period;modified_attributes;modified_attributes_list;name;pager;service_notification_period;service_notifications_enabled
;;;;;;check_mk dummy contact;1;;;;;24X7;1;1;1;0;;check_mk;;24X7;1
;;;;;;Nagios Admin;1;;;;nagios@localhost;24x7;1;1;1;0;;nagiosadmin;;24x7;1
```

## (5) How to Use HAP2 Nagios Livestatus

Please refer to [How to use HAP2](./HowToUse.md).
