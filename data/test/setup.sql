GRANT all on ndoutils.* TO ndoutils@localhost IDENTIFIED BY 'admin';
CREATE DATABASE IF NOT EXISTS hatohol_client;
GRANT ALL PRIVILEGES ON hatohol_client.* TO hatohol@localhost IDENTIFIED BY 'hatohol';
CREATE DATABASE IF NOT EXISTS test_hatohol_client;
GRANT ALL PRIVILEGES ON test_hatohol_client.* TO hatohol@localhost IDENTIFIED BY 'hatohol';
GRANT USAGE ON *.* TO ''@'localhost';
GRANT ALL PRIVILEGES ON `test\_%`.* TO ''@'localhost';
