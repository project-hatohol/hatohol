from centos:6.7

maintainer hatohol project

# install libraries for Hatohol
RUN yum install -y glib2-devel libsoup-devel sqlite-devel mysql-devel mysql-server \
  libuuid-devel qpid-cpp-client-devel MySQL-python httpd mod_wsgi python-argparse autoconf-archive
# setup mysql
RUN echo "NETWORKING=yes" > /etc/sysconfig/network
RUN rpm -ivh --force http://dl.fedoraproject.org/pub/epel/6/x86_64/epel-release-6-8.noarch.rpm
RUN yum install -y librabbitmq-devel wget
RUN wget -P /etc/yum.repos.d/ http://project-hatohol.github.io/repo/hatohol-el6.repo
# setup qpid
RUN yum install -y json-glib-devel qpid-cpp-server-devel
# setup build environment
RUN yum groupinstall -y 'Development tools'
# setup newer gcc toolchain
RUN cd /etc/yum.repos.d && wget http://people.centos.org/tru/devtools-2/devtools-2.repo
RUN yum install -y devtoolset-2-gcc devtoolset-2-binutils devtoolset-2-gcc-c++
RUN yum install -y Django
RUN rpm -Uvh http://sourceforge.net/projects/cutter/files/centos/cutter-release-1.1.0-0.noarch.rpm
RUN yum install -y cutter tar
# git clone
RUN git clone https://github.com/project-hatohol/hatohol.git ~/hatohol
# set CI terget to WERCKER_GIT_COMMIT
ADD ./GIT_REVISION /root/hatohol/wercker/
# change work dir
WORKDIR /root/hatohol
# checkout target branch
RUN git fetch origin
RUN git checkout -qf `cat ~/hatohol/wercker/GIT_REVISION`
# build
RUN libtoolize && autoreconf -i
RUN scl enable devtoolset-2 "./configure"
RUN scl enable devtoolset-2 "make -j `cat /proc/cpuinfo | grep processor | wc -l`"
RUN make dist -j `cat /proc/cpuinfo | grep processor | wc -l`
# rpm build
RUN yum install -y rpm-build
RUN scl enable devtoolset-2 "MAKEFLAGS=\"-j `cat /proc/cpuinfo | grep processor | wc -l`\" rpmbuild -tb hatohol-*.tar.bz2"
RUN scl enable devtoolset-2 "gcc --version"
