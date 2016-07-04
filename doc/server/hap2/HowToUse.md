How to Use HAP2
===============

## Install RabbitMQ server on CentOS 7

In this document, we use epel to install RabbitMQ server.

First, you need to setup EPEL by following command:

    # yum install epel-release

Then, you can install rabbitmq-server from EPEL repository by following command:

    # yum install rabbitmq-server

### How to start rabbitmq-server on CentOS 7

You need to permit 5672/tcp port with firewalld and turn off SELinux.

First, you need to permit 5672/tcp with firewall-cmd by the following commands:

    # firewall-cmd --add-port=5672/tcp --zone=public --permanent
    # firewall-cmd --add-port=5672/tcp --zone=public

And you need to disable SELinux by the following command:

    # setsebool -P nis_enabled 1

Finally, you can enable and start rabbitmq-server by the following commands:

    # systemctl enable rabbitmq-server
    # systemctl start rabbitmq-server

If you use TLS connection, please see [TLS Setting](#user-content-tls-setting)

## Install RabbitMQ server on Ubuntu 14.04

You can install rabbitmq-server via apt by following command:

    $ sudo apt-get install rabbitmq-server

Then, rabbitmq-server runs automatically.

## Prepare RabbitMQ server settings

In this document, it assumes that virtual host is `hatohol`, user and password is `hatohol`.
If you use CentOS 7, you need to perform the following command as root instead of
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

## Installation of HAP2

You need to install hatohol-hap2-zabbix with the following command on CentOS 7:

    # yum install -y hatohol-hap2-<plugin name>

Example.
    # yum install -y hatohol-hap2-zabbix

If you install hap2-nagios-livestatus, you need to install `python-mk-livestatus` module.
via pip with the following command:

    # pip install python-mk-livestatus

## Add each HAP2 to server types of WebUI menu

Each of the HAP2 plugins RPM install SQL file to /usr/share/hatohol/sql .

After install HAP2 plugins, you should read each of the SQL files and
add monitoring server types by hatohol-db-initiator command.

Please execute the following.

```
$ hatohol-db-initiator --db-user <YOUR_DB_USER> --db-password <YOUR_DB_PASSWORD>
```

Please confirm whether or not it succeeded by the hatohol-db-initiator command output.

```
Succeessfully loaded: /usr/bin/../share/hatohol/sql/90-server-type-<PLUGIN_NAME>.sql
```

If you can see `<Plugin name> (HAP2)` in `Monitoring Server` screen of WebUI, you can use HAP2!

## Add HAP2 to monitoring server.

You can add HAP2 to monitoring servers in `Monitoring Server` menu of WebUI.

Also you have to input `amqp://<user>:<password>@hostname/<vhost>` into BrokerURL.
These parameter should be replaced string that you input command for `$ sudo rabbitmqctl add_(user|vhost)`. If you execute commands same as in this document, you should input `amqp://hatohol:hatohol@localhost/hatohol`.


