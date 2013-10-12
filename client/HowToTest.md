Install packages (for Ubuntu 12.04)
-----------------------------------

    # apt-get npm
    # npm install -g mocha
    # npm install -g expect.js
    # apt-get install nodejs-legacy

make symbolic links
-------------------
    $ cd test/browser
    $ ln -s /usr/local/lib/node_modules/mocha/mocha.js
    $ ln -s /usr/local/lib/node_modules/mocha/mocha.css
    $ ln -s /usr/local/lib/node_modules/expect.js/expect.js

run djang with debug mode
-------------------------
    $ HATOHOL_DEBUG=1 ./manage.py runserver 0.0.0.0:8000

run test on the browser
-----------------------
Access the following URL.

http://localhost:8000/test/index.html

