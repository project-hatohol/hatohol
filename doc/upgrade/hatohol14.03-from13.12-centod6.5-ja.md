CentOS 6.5 (x86_64)での13.12から14.03へのRPMを用いたアップグレード方法
=====================================================================

Hatohol13.12のアップデート方法
-------------------------------
### Hatohol Serverの停止
アップデートするためにHatohol Serverを停止させます。

次のコマンドでHatohol Serverを停止させてください。

    # service hatohol stop

### Hatohol Server
以下のURLからRPMをダウンロードしてください。

- https://github.com/project-hatohol/hatohol-packages/raw/master/RPMS/14.03/hatohol-14.03-4.el6.x86_64.rpm

### Hatohol Client
以下のURLからRPMをダウンロードしてください。

- https://github.com/project-hatohol/hatohol-packages/raw/master/RPMS/14.03/hatohol-client-14.03-4.el6.x86_64.rpm

### アップデート方法
次のコマンドでアップデートしてください。

    # rpm -Uhv hatohol-14.03-4.el6.x86_64.rpm hatohol-client-14.03-4.el6.x86_64.rpm

Hatoholの再開
---------------
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

- http://192.168.1.1/

> ** 情報 **
> Hatohol14.03からはクライアントへアクセスするために、ホスト名のあとに"/viewer"を
> つける必要がなくなりました。
> 
> ** メモ **
> 現在、上記ページは、Google ChromeおよびFirefoxを使ってチェックされています。
> Internet Explorerを使用する場合は、ご使用のバージョンによっては、
> 表示レイアウトが崩れる場合があります。（IE11では正常に表示されることを確認しています）

