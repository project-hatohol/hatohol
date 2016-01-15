How to Use HAP2
===============

## Install RabbitMQ server on CentOS 7

First, you need to install erlang package.
In this document, we use epel to install RabbitMQ server.

First, you need to setup EPEL by following command:

    # wget -O /etc/yum.repos.d/epel-erlang.repo http://repos.fedorapeople.org/repos/peter/erlang/epel-erlang.repo

Then, you can install erlang from EPEL repository by following command:

    # yum install erlang

Finally, you can install rabbitmq-server package by following command:

    # rpm --import https://www.rabbitmq.com/rabbitmq-signing-key-public.asc
    # yum install rabbitmq-server-3.6.0-1.noarch.rpm

### How to start rabbitmq-server on CentOS 7

TBD.

## Install RabbitMQ server on Ubuntu 14.04

You can install rabbitmq-server via apt by following command:

    $ sudo apt-get install rabbitmq-server

Then, rabbitmq-server runs automatically.

## Prepare RabbitMQ server settings

In this document, it assumes that virtual host is `hatohol`, user and password is `hatohol`.

1. Create virtual host

    $ sudo rabbitmqctl add_vhost hatohol

2. Create user and password

    $ sudo rabbitmqctl add\_user hatohol hatohol

3. Set permissions to created user

    $ sudo rabbitmqctl set\_permissions -p hatohol hatohol ".*" ".*" ".*"
