Install packages (for Ubuntu 12.04)
-----------------------------------

    # apt-get install npm
    # npm install -g mocha
    # npm install -g expect.js
    # apt-get install nodejs-legacy

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

> ** Memo ** The set of the above environment variable make 'tasting' and 'test'
directories accessible.

run test on the browser
-----------------------
Access the following URL.

http://localhost:8000/test/index.html

