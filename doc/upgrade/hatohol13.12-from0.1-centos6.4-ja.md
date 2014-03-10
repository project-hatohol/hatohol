CentOS 6.4 (x86_64)での0.1から13.12へのRPMを用いたアップグレード方法
=====================================================================

必要な追加パッケージのインストール
-----------------------------------
### Django
Project HatoholはDjangoのRPMを提供します。

そのため、pipでインストールしたDjangoをアンインストールします。

次のコマンドでアンインストールしてください。

    # pip uninstall django

以下のURLからRPMをダウンロードしてください。

- https://github.com/project-hatohol/Django-for-distribution/raw/master/dist/Django-1.5.3-1.noarch.rpm

次のコマンドでインストールしてください。

    # rpm -Uhv Django-1.5.3-1.noarch.rpm

### 追加の依存パッケージ
Hatohol13.12ではHatohol0.1でのパッケージに加え、以下のパッケージをインストールする必要があります。
- libuuid
- MySQL-python
- mod_wsgi
- httpd

次のコマンドでインストールしてください

    # yum install libuuid MySQL-python mod_wsgi httpd

Hatohol13.12のアップデート方法
-------------------------------
### Hatohol Serverの停止
アップデートするためにHatohol Serverを停止させます。

次のコマンドでHatohol Serverを停止させてください。

    # service hatohol stop

### Hatohol Server
以下のURLからRPMをダウンロードしてください。

- https://github.com/project-hatohol/hatohol-packages/raw/master/RPMS/13.12/hatohol-13.12-1.el6.x86_64.rpm

### Hatohol Client
以下のURLからRPMをダウンロードしてください。

- https://github.com/project-hatohol/hatohol-packages/raw/master/RPMS/13.12/hatohol-client-13.12-1.el6.x86_64.rpm

### アップデート方法
次のコマンドでアップデートしてください。

    # rpm -Uhv hatohol-13.12-1.el6.x86_64.rpm hatohol-client-13.12-1.el6.x86_64.rpm

再セットアップ
---------------
### Hatohol cache DBの再指定
Hatohol13.12ではHatohol chache DBを指定する
環境変数'HATOHOL_DB_DIR'の記述箇所が変更になります。

環境変数'HATOHOL_DB_DIR'を指定するために/etc/hatohol/initrcファイルを作ります。

/etc/hatohol/ ディレクトリを作り、Hatohol cache DBのパスをinitrcファイルとして
/etc/hatohol/ ディレクトリに保存してください。

以下は、'/var/lib/hatohol'を指定する場合の例を示します。

    export HATOHOL_DB_DIR=/var/lib/hatohol

### Hatohol DBの再初期化

Hatohol13.12では新たにユーザ認証の機能を追加しました。
そのため、初期ユーザの設定を追加する必要があります。
また、Hatohol0.1でサーバ登録時に記載が必要だったサーバIDの記載は不必要になりました。

設定のテンプレートファイルを任意のディレクトリにコピーしてください。

    # cp /usr/share/hatohol/hatohol-config.dat.example ~/hatohol-config.dat

Zabbix serverとnagiosサーバの情報やHatoholにログインするときに必要な情報を
コピーした設定ファイルに追加してください。
いつくかのルールとサンプルがそのファイルの中にあります。

> *** 注意 ***
> サーバ設定の中のIDの記載は不必要になりましたので、消し忘れがないよう気をつけてください。

次のコマンド実行して設定をデータベースに反映してください。

    # hatohol-config-db-creator hatohol-config.dat

### Hatohol Clientの準備

Hatohol13.12では、Hatohol Client用のDBの準備を行う必要があります。
以下の手順を実行してください。

- Client用のDBを準備する

以下のコマンドをMySQLで実行してください。

    > CREATE DATABASE hatohol_client;
    > GRANT ALL PRIVILEGES ON hatohol_client.* TO hatohol@localhost IDENTIFIED BY 'hatohol';

- DB内にテーブルを追加する

以下のコマンドを実行してください。

    # /usr/libexec/hatohol/client/managy.py syncdb

### Hatohol Serverの開始

    # service hatohol start


Hatohol serverが正常に開始した場合、/var/log/messagesに下記のようなメッセージが記録されます。

    Oct  8 09:46:58 localhost hatohol[3038]: [INFO] <DBClientConfig.cc:336> Configuration DB Server: localhost, port: (default), DB: hatohol, User: hatohol, use password: yes
    Oct  8 09:46:58 localhost hatohol[3038]: [INFO] <main.cc:165> started hatohol server: ver. 13.12
    Oct  8 09:46:58 localhost hatohol[3038]: [INFO] <FaceRest.cc:121> started face-rest, port: 33194
    Oct  8 09:46:59 localhost hatohol[3038]: [INFO] <ArmZabbixAPI.cc:925> started: ArmZabbixAPI (server: testZbxSv1)
    Oct  8 09:47:01 localhost hatohol[3038]: [INFO] <ArmZabbixAPI.cc:925> started: ArmZabbixAPI (server: testZbxSv2)

### Hatohol情報の閲覧
例えば、Hatohol clientが192.168.1.1で動作している場合、
ブラウザを用いて次のURLを開いてください。

- http://192.168.1.1/viewer/

> ** メモ **
> 現在、上記ページは、Google ChromeおよびFirefoxを使ってチェックされています。
> Internet Explorerを使用する場合は、ご使用のバージョンによっては、
> 表示レイアウトが崩れる場合があります。（IE11では正常に表示されることを確認しています）
