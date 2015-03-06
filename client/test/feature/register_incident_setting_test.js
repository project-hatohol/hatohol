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

casper.test.begin('Register/Unregister incident settings test', function(test) {
  casper.start('http://0.0.0.0:8000/ajax_dashboard');
  casper.then(function() {util.login(test);});
  casper.then(function() {
    util.moveToServersPage(test);
    casper.then(function() {
      // register Monitoring Server (Zabbix)
      util.registerMonitoringServer(test,
                                    {serverType: 0,
                                     nickName: "zabbix",
                                     serverName: "test-zabbix",
                                     ipAddress: "127.0.0.1",
                                     userName: "admin",
                                     userPassword: "zabbix-admin"});
    });
  });
  casper.then(function() {util.moveToIncidentSettingsPage(test);});
  casper.then(function() {
    util.registerIncidentTrackerRedmine(test,
                                        {nickName: "testmine1",
                                         ipAddress: "http://127.0.0.1",
                                         projectId: "1",
                                         apiKey: "testredmine"});
  });
  casper.waitForSelector("form button#add-incident-setting-button",
    function success() {
      test.assertExists("form button#add-incident-setting-button");
      this.click("form button#add-incident-setting-button");
    },
    function fail() {
      test.assertExists("form button#add-incident-setting-button");
    });
  casper.then(function() {
    casper.evaluate(function() {
      $("#selectServerId").val("SELECT").change();
    });
  });
  casper.then(function() {
    this.evaluate(function() {
      $("#selectorMainTable").find("tr").last().click().change();
    });
  });
  casper.waitForSelector(".ui-dialog-buttonset > button",
    function success() {
      test.assertExists(".ui-dialog-buttonset > button",
                        "Select incident tracker server dialog button appeared when " +
                        "registering target incident settings.");
      this.evaluate(function() {
        $("div.ui-dialog-buttonset").attr("area-described-by","server-selector")
          .last().find("button").click().change();
      });
    },
    function fail() {
      test.assertExists(".ui-dialog-buttonset > button");
    });
  // add incident settings
  casper.waitForSelector(".ui-dialog-buttonset > button",
    function success() {
      test.assertExists(".ui-dialog-buttonset > button",
                        "Confirmation dialog button appeared when " +
                        "registering incident settings.");
      this.click(".ui-dialog-buttonset > button");
    },
    function fail() {
      test.assertExists(".ui-dialog-buttonset > button");
    });
  // close adding status dialog
  casper.waitForSelector("div.ui-dialog-buttonset button",
    function success() {
      test.assertExists("div.ui-dialog-buttonset button",
                        "Confirmation dialog appeared.");
      this.evaluate(function() {
        $("div.ui-dialog-buttonset button").click();
      });
    },
    function fail() {
      test.assertExists("div.ui-dialog-buttonset button");
    });
  // check delete-selector checkbox in incident setting
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
    this.echo("Cannot find .selectcheckbox in the monitoring servers table.");
  });

  casper.waitForSelector("form button#delete-incident-setting-button",
    function success() {
      test.assertExists("form button#delete-incident-setting-button",
                        "Found delete incident setting button.");
      this.click("form button#delete-incident-setting-button");
    },
    function fail() {
      test.assertExists("form button#delete-incident-setting-button");
    });
  // click Yes in delete incident settings dialog
  casper.waitForSelector("div.ui-dialog-buttonset button",
    function success() {
      test.assertExists("div.ui-dialog-buttonset button",
                        "Confirmation dialog appeared.");
      this.evaluate(function() {
        $("div.ui-dialog-buttonset button").next().click();
      });
    },
    function fail() {
      test.assertExists("div.ui-dialog-buttonset button");
    });
  // close result confirmation dialog
  casper.waitForSelector("div.ui-dialog-buttonset > button",
    function success() {
      this.click("div.ui-dialog-buttonset > button");
    },
    function fail() {
      test.assertExists("div.ui-dialog-buttonset > button");
    });
  casper.then(function() {util.unregisterIncidentTrackerRedmine(test);});
  casper.then(function() {
    util.moveToServersPage(test);
    util.unregisterMonitoringServer();
  });
  casper.then(function() {util.logout(test);});
  casper.run(function() {test.done();});
});
