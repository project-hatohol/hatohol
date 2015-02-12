var x = require('casper').selectXPath;
casper.options.viewportSize = {width: 1024, height: 768};
casper.on('page.error', function(msg, trace) {
   this.echo('Error: ' + msg, 'ERROR');
   for(var i=0; i<trace.length; i++) {
       var step = trace[i];
       this.echo('   ' + step.file + ' (line ' + step.line + ')', 'ERROR');
   }
});
casper.test.begin('Login/Logout test', function(test) {
  casper.start('http://0.0.0.0:8000/ajax_dashboard');
  casper.waitForSelector("input#inputUserName",
    function success() {
      this.sendKeys("input#inputUserName", "admin");
    },
    function fail() {
      test.assertExists("input#inputUserName");
    }
  );
  casper.waitForSelector("input#inputPassword",
    function success() {
      this.sendKeys("input#inputPassword", "hatohol");
    },
    function fail() {
      test.assertExists("input#inputPassword");
   }
  );

  casper.then(function() {
    this.click('input#loginFormSubmit');
    test.assertHttpStatus(200, 'It can logged in.');
  });

  casper.then(function() {
    casper.wait(200, function() {
      casper.log('should appear after 200ms', 'info');
      test.assertTextDoesntExist('None', 'None does not exist within the body when logged in.');
    });
  });

  casper.then(function() {
    this.click('#userProfileButton');
    this.waitForSelector('#logoutMenuItem', function() {
      this.click('#logoutMenuItem');
    });
    casper.wait(200, function() {
      test.assertTextExists('None', 'None exists within the body when logged out.');
    });
  });

  casper.run(function() {test.done();});
});
