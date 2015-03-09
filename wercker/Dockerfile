from centos:6.6

maintainer hatohol project

# install libraries for Hatohol
RUN yum install -y glib2-devel libsoup-devel sqlite-devel mysql-devel libuuid-devel \
  qpid-cpp-client-devel MySQL-python httpd mod_wsgi python-argparse

RUN rpm -ivh --force http://dl.fedoraproject.org/pub/epel/6/x86_64/epel-release-6-8.noarch.rpm
RUN yum install -y librabbitmq-devel wget
RUN wget -P /etc/yum.repos.d/ http://project-hatohol.github.io/repo/hatohol.repo
RUN yum install -y json-glib-devel qpid-cpp-server-devel
RUN yum groupinstall -y 'Development tools'
RUN yum install -y Django
# git clone
RUN git clone https://github.com/project-hatohol/hatohol.git ~/hatohol
# build
RUN cd ~/hatohol && libtoolize && autoreconf -i
RUN cd ~/hatohol && ./configure
RUN cd ~/hatohol && make -j `cat /proc/cpuinfo | grep processor | wc -l`
