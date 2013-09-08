
Command line options
--------------------
--face-rest-port N  
specify the port numer for the rest service.

--config-db-server server:[port]  
specify the data base server (and optionally port number) for config data.

--foreground  
run hatohol as a foreground process.

--pid-file-path  
specify the path of the pid file.

--enable-copy-on-demand  
enable the copy on demand.

Examples
--------
start hatohol as a foreground process without installing. The specification of the pid file enables non-root users to execute it.

    server $ LD_LIBRARY_PATH=mlpl/src/.libs:src/.libs HATOHOL_DB_PATH=~/tmp src/.libs/hatohol --pid-file-path ~/tmp/hatohol.pid --foreground
