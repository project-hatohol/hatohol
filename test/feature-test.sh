#/bin/sh
sudo make install
./test/launch-hatohol-for-test.sh && \
    LD_LIBRARY_PATH=/usr/local/lib:$LD_LIBRARY_PATH hatohol-db-initiator --db_user root --db_password '' && \
    casperjs test client/test/feature/*.js
mysql -uroot hatohol -e 'select * from users;'
