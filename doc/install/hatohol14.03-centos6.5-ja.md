CentOS 6.5 (x86_64)でのRPMを用いたインストール方法
============================================================

必要なパッケージのインストール
-------------------------------
### json-glib
次のコマンドでインストールしてください。

    # yum localinstall https://github.com/project-hatohol/json-glib-for-distribution/raw/master/RPMS/x86_64/json-glib-0.12.6-1PH.x86_64.rpm

### Django
次のコマンドでインストールしてください。

    # yum localinstall https://github.com/project-hatohol/Django-for-distribution/raw/master/dist/Django-1.5.3-1.noarch.rpm

### Hatohol Server
次のコマンドでインストールしてください。

    # yum localinstall https://github.com/project-hatohol/hatohol-packages/raw/master/RPMS/14.03/hatohol-14.03-4.el6.x86_64.rpm

### Hatohol Client
次のコマンドでインストールしてください。

    # yum localinstall https://github.com/project-hatohol/hatohol-packages/raw/master/RPMS/14.03/hatohol-client-14.03-4.el6.x86_64.rpm

> ** 情報 **
> 上記のコマンドによってインストールされる依存パッケージは以下のとおりです。
> - glib2
> - libsoup
> - sqlite
> - mysql
> - mysql-server
> - libuuid
> - MySQL-python
> - httpd
> - mod_wsgi


セットアップ
------------
### MySQL serverのセットアップ
すでにMySQL serverを使用している場合、このステップは省略できます。

    # chkconfig mysqld on
    # service mysqld start

### Hatohol cache DBのセットアップ
Hatohol cache DBのためのディレクトリを用意します。
このディレクトリのパスは任意です。
ここでは'/var/lib/hatohol'を使用する例を示します。

ディレクトリを作成します。

    # mkdir /var/lib/hatohol

このディレクトリを指定するために/etc/hatohol/initrcファイルを作り、
そのファイルに環境変数'HATOHOL_DB_DIR'を追加します。

以下の内容をinitrcファイルとして
/etc/hatoholディレクトリに保存してください。

    export HATOHOL_DB_DIR=/var/lib/hatohol

### Hatohol DBの初期化

設定のテンプレートファイルを任意のディレクトリにコピーしてください。

    # cp /usr/share/hatohol/hatohol-config.dat.example ~/hatohol-config.dat

Zabbix serverとnagiosサーバの情報やHatoholにログインするときに必要な情報を
コピーした設定ファイルに追加してください。
いつくかのルールとサンプルがそのファイルの中にあります。

次のコマンドを実行して設定をデータベースに反映してください。

    # hatohol-config-db-creator hatohol-config.dat

### Hatohol Clientの準備

Hatohol Client用のDBの準備を行います。
以下の手順を実行してください。

- Client用のDBを準備する

以下のコマンドをMySQLで実行してください。

    > CREATE DATABASE hatohol_client;
    > GRANT ALL PRIVILEGES ON hatohol_client.* TO hatohol@localhost IDENTIFIED BY 'hatohol';

- DB内にテーブルを追加する

以下のコマンドを実行してください。

    # /usr/libexec/hatohol/client/manage.py syncdb

### Hatohol Serverの開始

    # service hatohol start


Hatohol serverが正常に開始した場合、/var/log/messagesに下記のようなメッセージが記録されます。

    Oct  8 09:46:58 localhost hatohol[3038]: [INFO] <DBClientConfig.cc:336> Configuration DB Server: localhost, port: (default), DB: hatohol, User: hatohol, use password: yes
    Oct  8 09:46:58 localhost hatohol[3038]: [INFO] <main.cc:165> started hatohol server: ver. 0.1
    Oct  8 09:46:58 localhost hatohol[3038]: [INFO] <FaceRest.cc:121> started face-rest, port: 33194
    Oct  8 09:46:59 localhost hatohol[3038]: [INFO] <ArmZabbixAPI.cc:925> started: ArmZabbixAPI (server: testZbxSv1)
    Oct  8 09:47:01 localhost hatohol[3038]: [INFO] <ArmZabbixAPI.cc:925> started: ArmZabbixAPI (server: testZbxSv2)

> ** TROUBLE SHOOT ** Hatohol Server は現状、全てのログを syslog へ USER.INFO で出力します。USER.INFO は CentOS 6 デフォルトでは /var/log/messages にルーティングされています。

### Hatohol Clientの開始

    # service httpd start


Webブラウザを使ったアクセス
---------------------------
### SELinuxとiptablesの設定確認
デフォルトでは、SELinuxやiptablesのようないくつかのセキュリティ機構によって他のコンピュータからのアクセスが妨げられます。
必要に応じて、それらを解除しなければなりません。
> ** 警告 **
> 下記の設定を行うにあたり、セキュリティリスクについてよく理解してください。

現在のSELinuxの状態は次のコマンドで確認できます。

    # getenforce
    Enforcing

もし'Enforcing'であれば、次のコマンドで無効化できます。

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

- http://192.168.1.1/

hatohol-config.datで指定したユーザとパスワードでログインすることで、
各種画面の閲覧が可能になります。

> ** メモ **
> 現在、上記ページは、Google ChromeおよびFirefoxを使ってチェックされています。
> Internet Explorerを使用する場合は、ご使用のバージョンによっては、
> 表示レイアウトが崩れる場合があります。（IE11では正常に表示されることを確認しています）

