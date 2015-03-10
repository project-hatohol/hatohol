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

casper.test.begin('Register/Unregister log search system test', function(test) {
  var logSearchSystemURL = "http://search.example.com:10041/#/tables/Logs/search";
  casper.start('http://0.0.0.0:8000/ajax_dashboard');
  casper.then(function() {util.login(test);});
  casper.waitForSelector(x("//a[normalize-space(text())='ログ検索システム']"),
    function success() {
      test.assertExists(x("//a[normalize-space(text())='ログ検索システム']"));
      this.click(x("//a[normalize-space(text())='ログ検索システム']"));
    },
    function fail() {
      test.assertExists(x("//a[normalize-space(text())='ログ検索システム']"));
    });
  casper.waitForSelector("form button#add-log-search-system-button",
    function success() {
      test.assertExists("form button#add-log-search-system-button",
                        "Found add logserch system button item");
      this.click("form button#add-log-search-system-button");
    },
    function fail() {
      test.assertExists("form button#add-log-search-system-button");
    });
  casper.waitForSelector("input#editLogSearchSystemBaseURL",
    function success() {
      test.assertExists("input#editLogSearchSystemBaseURL");
      this.sendKeys("input#editLogSearchSystemBaseURL",
                    logSearchSystemURL);
    },
    function fail() {
      test.assertExists("input#editLogSearchSystemBaseURL");
    });
  // register log search system
  casper.waitForSelector(".ui-dialog-buttonset > button",
    function success() {
      test.assertExists(".ui-dialog-buttonset > button");
      this.click(".ui-dialog-buttonset > button");
    },
    function fail() {
      test.assertExists(".ui-dialog-buttonset > button");
    });
  // close comfirm dialog
  casper.waitForSelector("div.ui-dialog-buttonset > button",
    function success() {
      test.assertExists("div.ui-dialog-buttonset > button",
                        "Confirmation dialog button appeared after registering.");
      this.click("div.ui-dialog-buttonset > button");
    },
    function fail() {
      test.assertExists("div.ui-dialog-buttonset > button");
    });
  casper.then(function() {util.moveToDashboardPage(test);});
  // confirm log search system search form appeared in dashboard
  casper.then(function() {
    test.assertExists("#tblLogSearch");
    test.assertTextExists(logSearchSystemURL,
                          "#tblLogSearch contains " + logSearchSystemURL);
  });
  casper.then(function() {util.moveToLogSearchSystemPage(test);});

  // check delete-selector check box in log search system
  casper.waitFor(function() {
    return this.evaluate(function() {
      return document.querySelectorAll("div.ui-dialog").length < 1;
    });
  }, function then() {
    this.evaluate(function() {
      $("tr:last").find(".selectcheckbox").click();
      return true;
    });
  }, function timeout() {
    this.echo("Oops, confirmation dialog dose not to be closed.");
  });

  casper.waitForSelector("form button#delete-log-search-system-button",
    function success() {
      test.assertExists("form button#delete-log-search-system-button",
                        "Found delete log search system button.");
      this.click("form button#delete-log-search-system-button");
    },
    function fail() {
      test.assertExists("form button#delete-log-search-system-button");
    });
  casper.waitForSelector("div.ui-dialog-buttonset button",
    function success() {
      test.assertExists("div.ui-dialog-buttonset button",
                        "Confirmation dialog appeared.");
      this.evaluate(function() {
        $("div.ui-dialog-buttonset button").next().click();
      });
    },
    function fail() {
      test.assertExists("form button#delete-server-button");
    });
  casper.then(function() {util.logout(test);});
  casper.run(function() {test.done();});
});
