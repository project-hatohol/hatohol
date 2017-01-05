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
  var incidentTracker = {nickName: "testredmine",
                        baseURL: "http://127.0.0.1",
                        projectId: "test-tracker-project",
                        key: "test-tracker-key1"};
  var editedIncidentTracker = {nickName: "editedRedmine",
                               baseURL: "http://127.0.1.1",
                               projectId: "edit-tracker-project",
                               key: "edit-tracker-key1"};
  casper.start('http://0.0.0.0:8000/ajax_dashboard');
  casper.then(function() {util.login(test);});
  // move to incident setting page
  util.openSettingMenu(test);
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
           this.sendKeys("input#editIncidentTrackerNickname",
                         incidentTracker.nickName);
       },
       function fail() {
           test.assertExists("input#editIncidentTrackerNickname");
   });
   casper.waitForSelector("input#editIncidentTrackerBaseURL",
       function success() {
           this.sendKeys("input#editIncidentTrackerBaseURL",
                         incidentTracker.baseURL);
       },
       function fail() {
           test.assertExists("input#editIncidentTrackerBaseURL");
   });
   casper.waitForSelector("input#editIncidentTrackerProjectId",
       function success() {
           this.sendKeys("input#editIncidentTrackerProjectId",
                         incidentTracker.projectId);
       },
       function fail() {
           test.assertExists("input#editIncidentTrackerProjectId");
   });
   casper.waitForSelector("input#editIncidentTrackerUserName",
       function success() {
           this.sendKeys("input#editIncidentTrackerUserName",
                         incidentTracker.key);
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

  // check DOMNodeInserted event
  casper.waitFor(function() {
    return this.evaluate(function() {
      return $(document).on("DOMNodeInserted",
                            "table#incidentTrackersEditorMainTable tr",
                            function() {return true;});
    });
  }, function then() {
    test.assertTextExists(incidentTracker.nickName,
                          "Registered incident tracker's nickName \""
                          + incidentTracker.nickName +
                          "\" exists in the incdent servers table.");
    this.evaluate(function() {
      $(document).off("DOMNodeInserted", "table#incidentTrackersEditorMainTable tr");
    });
  }, function timeout() {
    this.echo("Oops, table element does not to be newly created.");
  });

  // edit incident server setting
  casper.waitForSelector("form input[type=button][value='編集']",
     function success() {
       return this.evaluate(function() {
         $("input.editIncidentTracker").last().click();
       });
     },
     function fail() {
       test.assertExists("form input[type=button][value='編集']");
     });
  casper.waitForSelector("input#editIncidentTrackerNickname",
    function success() {
      test.assertFieldCSS("input#editIncidentTrackerNickname",
                          incidentTracker.nickName,
                          "Registered incident tracker server's nickName: \"" +
                          incidentTracker.nickName +
                          "\" exists in the edit monitoring server dialog" +
                          " input#editIncidentTrackerNickname.");
    },
    function fail() {
      test.assertExists("input#editIncidentTrackerNickname");
    });
  casper.waitForSelector("input#editIncidentTrackerNickname",
    function success() {
      this.sendKeys("input#editIncidentTrackerNickname",
                    editedIncidentTracker.nickName, {reset: true});
    },
    function fail() {
      test.assertExists("input#editIncidentTrackerNickname");
    });
  casper.waitForSelector("div.ui-dialog-buttonset > button",
    function success() {
      test.assertExists("div.ui-dialog-buttonset > button",
                        "Confirmation dialog button appeared after registering.");
      this.click("div.ui-dialog-buttonset > button");
    },
    function fail() {
      test.assertExists("div.ui-dialog-buttonset > button");
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
  // assert edit incident tracker server settings
  casper.waitFor(function() {
    return this.evaluate(function() {
      return document.querySelectorAll("div.ui-dialog").length < 2;
    });
  }, function then() {
    test.assertTextExists(editedIncidentTracker.nickName,
                          "Edited incident tracker server's nickName \""
                          + editedIncidentTracker.nickName + "\" is found.");
  }, function timeout() {
    this.echo("Oops, incident tracker server settings seems not to be edited.");
  });
  // start to delete incident tracker server settings
  casper.then(function() {
    this.evaluate(function() {
      $("tr:last").find(".incidentTrackerSelectCheckbox").click();
      return true;
    });
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
  casper.waitFor(function() {
    return this.evaluate(function() {
      return $(document).on("DOMNodeRemoved",
                            "table#incidentTrackersEditorMainTable tr",
                            function() {return true;});
    });
  }, function then() {
    test.assertTextDoesntExist(editedIncidentTracker.nickName,
                               "Registered incident tracker's nickName \""
                               + editedIncidentTracker.nickName +
                               "\" dose not exist in the user table.");
    this.evaluate(function() {
      $(document).off("DOMNodeRemoved", "table#incidentTrackersEditorMainTable tr");
    });
  }, function timeout() {
    this.echo("Oops, newly created table element does not to be deleted.");
  });
  casper.then(function() {util.logout(test);});
  casper.run(function() {test.done();});
});
