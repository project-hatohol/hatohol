var x = require("casper").selectXPath;
var util = require("feature_test_utils");
casper.options.viewportSize = {width: 1024, height: 768};
casper.on("page.error", function(msg, trace) {
  this.echo("Error: " + msg, "ERROR");
  for(var i = 0; i < trace.length; i++) {
    var step = trace[i];
    this.echo("   " + step.file + " (line " + step.line + ")", "ERROR");
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

  casper.waitFor(function() {
    return this.evaluate(function() {
      return $(document).on("DOMSubtreeModified", "div#update-time",
                            function() {return true;});
    });
  }, function then() {
    casper.then(function() {
      test.assertTextDoesntExist('None', 'None does not exist within the body when logged in.');
      this.evaluate(function() {
        $(document).off("DOMSubtreeModified", "div#update-time");
      });
    });
  }, function timeout() {
    this.echo("Oops, it seems not to be logged in.");
  });

  casper.then(function() {
    this.click('#userProfileButton');
    this.waitForSelector('#logoutMenuItem', function() {
      this.click('#logoutMenuItem');
    });
  });

  casper.waitFor(function() {
    return this.evaluate(function() {
      return $(document).on("DOMSubtreeModified", "div#update-time",
                            function() {return true;});
    });
  }, function then() {
    casper.then(function() {
      test.assertTextExists('None', 'None exists within the body when logged out.');
      this.evaluate(function() {
        $(document).off("DOMSubtreeModified", "div#update-time");
      });
    });
  }, function timeout() {
    this.echo("Oops, it seems not to be logged out.");
  });

  casper.run(function() {test.done();});
});
