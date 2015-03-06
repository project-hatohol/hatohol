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

casper.test.begin('Register/Unregister incident tracker(Redmine) test', function(test) {
  casper.start('http://0.0.0.0:8000/ajax_dashboard');
  casper.then(function() {util.login(test);});
  // move to incident setting page
  casper.waitForSelector(x("//a[normalize-space(text())='インシデント管理']"),
    function success() {
      test.assertExists(x("//a[normalize-space(text())='インシデント管理']",
                          "Found 'incident settings' menu"));
      this.click(x("//a[normalize-space(text())='インシデント管理']"));
    },
    function fail() {
      test.assertExists(x("//a[normalize-space(text())='インシデント管理']"));
    });
  casper.then(function() {
    casper.waitForUrl(/.*ajax_incident_settings/, function() {
      test.assertUrlMatch(/.*ajax_incident_settings/,
                          "Move into incident settings page.");
    });
  });
  // create incident tracker servers
  casper.waitForSelector("button#edit-incident-trackers-button",
    function success() {
      test.assertExists("button#edit-incident-trackers-button",
                        "Found adding incident trackers button item");
      this.click("button#edit-incident-trackers-button");
    },
    function fail() {
      test.assertExists("form button#add-incident-trackers-button");
    });
  casper.waitForSelector("form input[type=button][value='追加']",
     function success() {
       test.assertExists("form input[type=button][value='追加']");
       this.click("form input[type=button][value='追加']");
     },
     function fail() {
       test.assertExists("form input[type=button][value='追加']");
     });
   casper.waitForSelector("input#editIncidentTrackerNickname",
       function success() {
           test.assertExists("input#editIncidentTrackerNickname");
           this.click("input#editIncidentTrackerNickname");
       },
       function fail() {
           test.assertExists("input#editIncidentTrackerNickname");
   });
   casper.waitForSelector("input#editIncidentTrackerNickname",
       function success() {
           this.sendKeys("input#editIncidentTrackerNickname", "testredmine");
       },
       function fail() {
           test.assertExists("input#editIncidentTrackerNickname");
   });
   casper.waitForSelector("input#editIncidentTrackerBaseURL",
       function success() {
           this.sendKeys("input#editIncidentTrackerBaseURL", "http://127.0.0.1");
       },
       function fail() {
           test.assertExists("input#editIncidentTrackerBaseURL");
   });
   casper.waitForSelector("input#editIncidentTrackerProjectId",
       function success() {
           this.sendKeys("input#editIncidentTrackerProjectId", "1");
       },
       function fail() {
           test.assertExists("input#editIncidentTrackerProjectId");
   });
   casper.waitForSelector("input#editIncidentTrackerUserName",
       function success() {
           this.sendKeys("input#editIncidentTrackerUserName", "testkey1");
       },
       function fail() {
           test.assertExists("input#editIncidentTrackerUserName");
   });
  casper.waitForSelector(".ui-dialog-buttonset > button",
    function success() {
      test.assertExists(".ui-dialog-buttonset > button",
                        "Confirmation dialog button appeared when " +
                        "registering target incident trackers server.");
      this.evaluate(function() {
        $("#server-selector").find("table#selectorMainTable").find("tr").last()
          .click().change();
        $("div.ui-dialog-buttonset").attr("area-described-by","server-selector")
          .last().find("button").click().change();
      });
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
        $("div.ui-dialog-buttonset button").last().click();
      });
    },
    function fail() {
      test.assertExists("div.ui-dialog-buttonset button");
    });

  // check delete-selector check box in incident trackers server
  casper.waitFor(function() {
    return this.evaluate(function() {
      return document.querySelectorAll("div.ui-dialog").length < 2;
    });
  }, function then() {
    this.evaluate(function() {
      $("tr:last").find(".incidentTrackerSelectCheckbox").click();
      return true;
    });
  }, function timeout() {
    this.echo("Cannot find .incidentTrackerSelectCheckbox in the incident servers table.");
  });
  casper.waitForSelector("input#deleteIncidentTrackersButton",
    function success() {
      test.assertExists("input#deleteIncidentTrackersButton",
                        "Found delete incident trackers button.");
      this.click("input#deleteIncidentTrackersButton");
    },
    function fail() {
      test.assertExists("input#deleteIncidentTrackersButton");
    });

  // delete incident trackers server
  casper.waitForSelector("div.ui-dialog-buttonset button",
    function success() {
      test.assertExists("div.ui-dialog-buttonset button",
                        "Confirmation deleting dialog appeared.");
      this.evaluate(function() {
        $("div.ui-dialog-buttonset button").last().click();
      });
    },
    function fail() {
      test.assertExists("div.ui-dialog-buttonset button");
    });

  // close result confirmation dialog
  casper.waitForSelector("div.ui-dialog-buttonset button",
    function success() {
      test.assertExists("div.ui-dialog-buttonset button",
                        "Confirmation dialog appeare, and then close it.");
      this.evaluate(function() {
        $("div.ui-dialog-buttonset button").last().click();
      });
    },
    function fail() {
      test.assertExists("div.ui-dialog-buttonset button");
    });
  casper.then(function() {util.logout(test);});
  casper.run(function() {test.done();});
});
