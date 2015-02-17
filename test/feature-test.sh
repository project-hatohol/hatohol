#/bin/sh
sudo make install
LD_LIBRARY_PATH=/usr/local/lib:$LD_LIBRARY_PATH hatohol-db-initiator --db_user root --db_password ''
sleep 5
test/launch-hatohol-for-test.sh && casperjs test client/test/feature/*.js"
