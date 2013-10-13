if WITH_JS_TEST

TARGET_FILES = \
	node_modules \
	browser/mocha.js browser/mocha.css browser/expect.js

noinst_DATA = $(TARGET_FILES)
endif

node_modules:
	ln -s `npm root -g`

browser/mocha.js: node_modules
	ln -s ../node_modules/mocha/mocha.js $@

browser/mocha.css: node_modules
	ln -s ../node_modules/mocha/mocha.css $@

browser/expect.js: node_modules
	ln -s ../node_modules/expect.js/expect.js $@

clean-local:
	-rm -fr $(TARGET_FILES)
