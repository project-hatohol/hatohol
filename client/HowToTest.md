Install packages (for Ubuntu 12.04)
-----------------------------------

    # apt-get install npm
    # npm install -g mocha
    # npm install -g expect.js
    # npm install -g sinon
    # apt-get install nodejs-legacy
    # npm install -g mocha-phantomjs phantomjs
    # pip install django==1.5.4
    # pip install mysql-python

Install packages (for Debian 7)
-------------------------------

    # apt-get install checkinstall
    $ wget -N http://nodejs.org/dist/node-latest.tar.gz
    $ tar xzvf node-latest.tar.gz && cd node-v* 
    $ checkinstall -y --install=no --pkgversion 0.10.24 # Replace with current version number.
    # sudo dpkg -i node_*
    # curl https://npmjs.org/install.sh | sudo sh

The above instruction is an excerpt from https://github.com/joyent/node/wiki/Installing-Node.js-via-package-manager

    # npm install -g mocha
    # npm install -g expect.js
    # npm install -g mocha-phantomjs phantomjs
    # pip install django==1.5.4

make symbolic links
-------------------
Execute ./configure on the top directory. If the above packages are detected,
the following summary will be shown.

<pre>
checking for sqlite3... yes
checking for mocha... yes
checking for npm... yes
configure: creating ./config.status
<snip>
Configure Result:

  C Unit test (cutter) : yes
  JS Unit test         : yes
</pre>

Then, when 'make' is executed, the following links are created.

    test/node_modules
    browser/mocha.js
    browser/mocha.css
    browser/expect.js

run Hatohol with a debug mode
-----------------------------
run Hatohol server with --test-mode  
Ex.)

    $ LD_LIBRARY_PATH=mlpl/src/.libs:src/.libs HATOHOL_DB_DIR=~/tmp src/.libs/hatohol --pid-file-path ~/tmp/hatohol.pid --foreground --test-mode

run djang with debug mode (set an environment variable: HATOHOL_DEBUG=1)

    $ HATOHOL_DEBUG=1 ./manage.py runserver 0.0.0.0:8000

or if you want to use a different database on a test, you can specify the test settings that use the database named 'test_hatohol_client'.

    $ DJANGO_SETTINGS_MODULE=test.python.testsettings HATOHOL_DEBUG=1 ./manage.py runserver 0.0.0.0:8008

> ** Memo ** The set of the above environment variable make 'tasting' and 'test'
directories accessible.

> ** Hint ** The above two examples for client (mange.py) that use different ports can be executed at the same time.

run test on the browser
-----------------------
Access the following URL.

http://localhost:8000/test/index.html

