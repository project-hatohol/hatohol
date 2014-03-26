if WITH_JS_TEST

TARGET_FILES = \
	node_modules \
	browser/mocha.js browser/mocha.css browser/expect.js browser/sinon.js

noinst_DATA = $(TARGET_FILES)
endif

node_modules:
	ln -sf `npm root -g`

browser/mocha.js:
	ln -sf ../node_modules/mocha/mocha.js $@

browser/mocha.css:
	ln -sf ../node_modules/mocha/mocha.css $@

browser/expect.js:
	ln -sf ../node_modules/expect.js/index.js $@

browser/sinon.js:
	ln -sf ../node_modules/sinon/pkg/sinon.js $@

clean-local:
	-rm -fr $(TARGET_FILES)
