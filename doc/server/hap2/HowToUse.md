How to Use HAP2
===============

## Install RabbitMQ server on CentOS 7

First, you need to install erlang package.
In this document, we use epel to install RabbitMQ server.

First, you need to setup EPEL by following command:

    # wget -O /etc/yum.repos.d/epel-erlang.repo http://repos.fedorapeople.org/repos/peter/erlang/epel-erlang.repo

Then, you can install erlang from EPEL repository by following command:

    # yum install erlang

Finally, you can install rabbitmq-server package by following command
after downloading the server package from [RabbitMQ official site](https://www.rabbitmq.com/install-rpm.html):

    # rpm --import https://www.rabbitmq.com/rabbitmq-signing-key-public.asc
    # yum install rabbitmq-server-3.6.0-1.noarch.rpm

### How to start rabbitmq-server on CentOS 7

You need to permit 5672/tcp port with firewalld and turn off SELinux.

First, you need to permit 5672/tcp with firewall-cmd by the following commands:

    # firewall-cmd --add-port=5672/tcp --zone=public --permanent
    # firewall-cmd --add-port=5672/tcp --zone=public

And you need to disable SELinux by the following command:

    # setsebool -P nis_enabled 1

Finally, you can enable and start rabbitmq-server by the following commands:

    # systemctl enable rabbitmq-server
    # systemctl rabbitmq-server start

## Install RabbitMQ server on Ubuntu 14.04

You can install rabbitmq-server via apt by following command:

    $ sudo apt-get install rabbitmq-server

Then, rabbitmq-server runs automatically.

## Prepare RabbitMQ server settings

In this document, it assumes that virtual host is `hatohol`, user and password is `hatohol`.
If you use CentOS 7, you need to perform following command as root instead of
a user who belongs to `sudo` group.

First, create virtual host

```bash
$ sudo rabbitmqctl add_vhost hatohol
```

And then, create user and password

```bash
$ sudo rabbitmqctl add_user hatohol hatohol
```

Finally, set permissions to created user

```bash
$ sudo rabbitmqctl set_permissions -p hatohol hatohol ".*" ".*" ".*"
```

## Install hap2 plugin dependent python modules

### On CentOS 7

You need to install hap2 dependent packages via pip:

    # pip install pika daemon

If you want to use hap2-nagios-livestatus, you need to install `python-mk-livestatus`
via pip with the following command:

    # pip install python-mk-livestatus

### On Ubuntu 14.04

You need to install hap2 dependent packages via apt-get with the following command:

    $ sudo apt-get install python-pika python-daemon

If you want to use hap2-nagios-livestatus, you need to install `python-mk-livestatus`
via pip with the following command:

    $ sudo pip install python-mk-livestatus

## Starting HAP2 Tips

### HAP2 Zabbix

You should input `http://<servername or ip>/zabbix/api_jsonrpc.php` into
"Zabbix API URL" instead of `<servername or ip>` simply.

### HAP2 Nagios Livestatus

HAP2 Nagios Livestatus depends `nagios-mk-livestatus` package.
You should install this dependent package on Nagios node and its node's Nagios proicess
before you start to use this HAP2.
