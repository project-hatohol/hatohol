Install packages (for Ubuntu 12.04)
-----------------------------------

    # apt-get npm
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

run djang with debug mode
-------------------------
    $ HATOHOL_DEBUG=1 ./manage.py runserver 0.0.0.0:8000

> ** Memo ** The set of the above environment variable make 'tasting' and 'test'
directories accessible.

run test on the browser
-----------------------
Access the following URL.

http://localhost:8000/test/index.html

