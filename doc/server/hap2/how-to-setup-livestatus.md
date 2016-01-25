# How to setup Livestatus API
-----------------------------------------------------------------------------------

You should install livestatus module, if you use hap2_nagios_livestatus.py plugin.

This document describe for how to install livestatus module on CentOS 7 and Ubuntu 14.04 LTS.

This document is written with the assumption that Nagios is installed.

## (1) Install the livestatus module package

### CentOS 7

    # yum install epel-release
    # yum install check-mk-livestatus 

### Ubuntu 14.04 LTS

    # apt-get install check-mk-livestatus

## (2) Check livestauts library path

You execute the following command on each OS to check the path of "livestatus.o".

The path is replaced the next section's LIVESTATUS_LIB_PATH.

### CentOS 7

    # rpm -ql check-mk-livestatus

### Ubuntu

    # dpkg -L check-mk-livestatus

## (3) Edit /etc/nagios/nagios.cfg to use an livestatus broker module

Add the following line to /etc/nagios/nagios.cfg.

Livestatus module make AF UNIX socket file.

ANY_SOCKET_FILE_PATH designates this socket file path.

```
broker_module=LIVESTATUS_LIB_FILE_PATH config_file=ANY_SOCKET_FILE_PATH
```

## (3') Setup xinetd

If you want to activate hap2_nagios_livestatus.py outside the nagios local machine, 

you should setup the xinetd. This section describe how to setup xinetd.

### Installation to CentOS 7

    # yum install xinetd

### Installation to Ubuntu 14.04 LTS

    # apt-get install xinetd

### Put livestatus configuration file

The following is an example configuration file.
You put it to /etc/xinetd.d/livestatus .

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

### xinetd start

    # service xinetd start


## (4) Start nagios

    # service nagios start

## (4') Test

You can test whether livestatus module is operating correctly by the following command.

    # echo 'GET contacts' | unixcat ANY_FILE_PATH

If you confirm as the following output, livestatus module setting is completed.

## (5) How to Use HAP2 Nagios Livestatus

Please refer to [How to use HAP2](./HowToUse.md).
