CentOS 6.4 (x86_64)でのRPMを用いたインストール方法
============================================================

[English](hatohol0.1-centos6.4.md)

必要なパッケージのインストール
-------------------------------
### json-glib
以下のURLからRPMをダウンロードしてください。

- https://github.com/project-hatohol/json-glib-for-distribution/blob/master/RPMS/x86_64/json-glib-0.12.6-1PH.x86_64.rpm?raw=true

次のようにインストールしてください。

    # rpm -Uhv json-glib-0.12.6-1PH.x86_64.rpm

### bootstrap
以下のURLからRPMをダウンロードしてください。

- https://github.com/project-hatohol/bootstrap-for-hatohol/blob/master/RPMS/x86_64/bootstrap-for-hatohol-2.3.2-1PH.x86_64.rpm?raw=true

次のようにインストールしてください。

    # rpm -Uhv bootstrap-for-hatohol-2.3.2-1PH.x86_64.rpm

### 依存パッケージ
libsoupとMySQLをインストールします。

    # yum install libsoup mysql

Djangoをインストールします。

    # yum install python-setuptools
    # easy_install pip
    # pip install django

### Hatohol Server
以下のURLからRPMをダウンロードしてください。

- https://github.com/project-hatohol/hatohol-packages/blob/master/RPMS/hatohol-0.1-1.el6.x86_64.rpm?raw=true

次のようにインストールしてください。

    # rpm -Uhv hatohol-0.1-1.el6.x86_64.rpm

### Hatohol Client
以下のURLからRPMをダウンロードしてください。

- https://github.com/project-hatohol/hatohol-packages/blob/master/RPMS/hatohol-client-0.1-1.el6.x86_64.rpm?raw=true

次のようにインストールしてください。

    # rpm -Uhv hatohol-client-0.1-1.el6.x86_64.rpm

セットアップ
------------
### MySQL serverのインストール
すでにMySQL serverを使用している場合、このステップは省略できます。

    # yum install mysql-server
    # chkconfig mysqld on
    # service mysqld start

### Hatohol cache DBのセットアップ
Hatohol cache DBのためのディレクトリを用意します。ここでは例として'/var/lib/hatohol'を使用します。

必要に応じて、ディレクトリを作成します。

    # mkdir /var/lib/hatohol

このディレクトリを指定するために/etc/init.d/hatoholに環境変数'HATOHOL_DB_DIR'を追加します。

     PROG=/usr/sbin/$NAME
     OPTIONS="--config-db-server localhost"
     PROGNAME=`basename $PROG`
    +export HATOHOL_DB_DIR=/var/lib/hatohol
 
     [ -f $PROG ]|| exit 0

> ** メモ ** 先頭の'+'印は、新たに追加される行を意味します。

### Hatohol DBの初期化

設定のテンプレートファイルを任意のディレクトリにコピーしてください。

    # cp /usr/share/hatohol/hatohol-config.dat.example ~/hatohol-config.dat

Zabbix serverとnagiosサーバの情報をコピーした設定ファイルに追加してください。
いつくかのルールとサンプルがそのファイルの中にあります。

以下のように設定をデータベースに反映してください。

    # hatohol-config-db-creator hatohol-config.dat

### Hatohol Serverの開始

    # service hatohol start


Hatohol serverが正常に開始した場合、/var/log/messagesに下記のようなメッセージが記録されます。

    Oct  8 09:46:58 localhost hatohol[3038]: [INFO] <DBClientConfig.cc:336> Configuration DB Server: localhost, port: (default), DB: hatohol, User: hatohol, use password: yes
    Oct  8 09:46:58 localhost hatohol[3038]: [INFO] <main.cc:165> started hatohol server: ver. 0.1
    Oct  8 09:46:58 localhost hatohol[3038]: [INFO] <FaceRest.cc:121> started face-rest, port: 33194
    Oct  8 09:46:59 localhost hatohol[3038]: [INFO] <ArmZabbixAPI.cc:925> started: ArmZabbixAPI (server: testZbxSv1)
    Oct  8 09:47:01 localhost hatohol[3038]: [INFO] <ArmZabbixAPI.cc:925> started: ArmZabbixAPI (server: testZbxSv2)

### Hatohol Clientの開始

    # /usr/libexec/hatohol/client/manage.py runserver 0.0.0.0:8000

> ** メモ **
> アクセス可能な範囲は、第2引数によって制限できます。上記の例ではHatohol clientは任意のコンピュータに応答します。

Webブラウザを使ったアクセス
---------------------------
### SELinuxとiptablesの設定確認
デフォルトでは、SELinuxやiptablesのようないくつかのセキュリティ機構によって他のコンピュータからのアクセスが妨げられます。
必要に応じて、それらを解除しなければなりません。
> ** 警告 **
> 下記の設定を行うにあたり、セキュリティリスクについてよく理解してください。

現在のSELinuxの状態は次のように確認できます。

    # getenforce
    Enforcing

もし'Enforcing'であれば、次のように無効化できます。

    # setenforce 0
    # getenforce
    Permissive

> ** ヒント **
> /etc/selinux/configを編集することで、恒久的な無効化を行えます。

iptablesについては、/etc/sysconfig/iptablesの編集により許可ポートを追加できます。
下記は、8000番ポートを許可する例です。

      -A INPUT -p icmp -j ACCEPT
      -A INPUT -i lo -j ACCEPT
      -A INPUT -m state --state NEW -m tcp -p tcp --dport 22 -j ACCEPT
     +-A INPUT -m state --state NEW -p tcp --dport 8000 -j ACCEPT
      -A INPUT -j REJECT --reject-with icmp-host-prohibited
      -A FORWARD -j REJECT --reject-with icmp-host-prohibited

> ** メモ ** 先頭の'+'印は、新たに追加される行を意味します。

次のコマンドはiptablesの設定をリロードします。

    # service iptables restart

### Hatohol情報の閲覧
例えば、Hatohol clientが192.168.1.1で動作している場合、
ブラウザを用いて次のURLを開いてください。

- http://192.168.1.1:8000/viewer/

> ** メモ **
> 現在、上記ページは、Google Chromeを使ってチェックされています。
