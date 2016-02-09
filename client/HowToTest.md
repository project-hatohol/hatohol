Install packages (for Ubuntu 12.04)
-----------------------------------

    # apt-get install npm nodejs-legacy
    # pip install django==1.5.4
    # pip install mysql-python
    $ cd /path/to/hatohol-source-directory
    $ npm install

Install packages (for Debian 7)
-------------------------------

    # apt-get install checkinstall
    $ wget -N http://nodejs.org/dist/node-latest.tar.gz
    $ tar xzvf node-latest.tar.gz && cd node-v*
    $ checkinstall -y --install=no --pkgversion 0.10.24 # Replace with current version number.
    # sudo dpkg -i node_*
    # curl https://npmjs.org/install.sh | sudo sh

The above instruction is an excerpt from https://github.com/joyent/node/wiki/Installing-Node.js-via-package-manager

    # pip install django==1.5.4
    $ cd /path/to/hatohol-source-directory
    $ npm install

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

    $ LD_LIBRARY_PATH=mlpl/src/.libs:src/.libs HATOHOL_DB_DIR=~/tmp src/.libs/hatohol --pid-file ~/tmp/hatohol.pid --foreground --test-mode

run djang with debug mode (set an environment variable: HATOHOL_DEBUG=1)

    $ HATOHOL_DEBUG=1 ./manage.py runserver 0.0.0.0:8000

or if you want to use a different database on a test, you can specify the test settings that use the database named 'test_hatohol_client'.

    $ DJANGO_SETTINGS_MODULE=test.python.testsettings HATOHOL_DEBUG=1 ./manage.py runserver 0.0.0.0:8008

> ** Memo ** The set of the above environment variable make 'tasting' and 'test'
directories accessible.

> ** Hint ** The above two examples for client (mange.py) that use different ports can be executed at the same time.

run unit test on the browser
----------------------------
Access the following URL.

http://localhost:8000/test/index.html

run feature test on commandline
-------------------------------

You can specify test files with wildcard.
Run all feature tests:

    $ casperjs test client/test/feature/*_test.js

Or, run feature test(s), respectively:

    $ casperjs test client/test/feature/feature1_test.js client/test/feature/feature2_test.js

> NOTE: If you use non-Japanese langage environment Linux OS, you need to specify following environment variables before running feature test(s):

    $ export LANG=ja_JP.utf8 LC_ALL=ja_JP.UTF-8

> Hint: Casper.js test generator which is called [resurrectio](https://github.com/ebrehault/resurrectio) is provided as Google Chrome extension.
> You can use this Google Chrome extension to generate feature test skelton. This extension's Casper.js target is Casper.js **1.1-beta** series. Casper.js 1.0 or older dose *NOT* support.

> Memo: Casper.js has the method which can evaluate an expression in the current page DOM context.
> You can use jQuery code inside evaluate function like this:

    casper.then(function() {
      this.evaluate(function() {
        $("div.ui-dialog-buttonset button").click();
      });
    }
