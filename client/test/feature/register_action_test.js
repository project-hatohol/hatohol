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

casper.on('remote.message', function(msg) {
  this.echo(msg);
});

casper.test.begin('Register/Unregister action test', function(test) {
  var actionCommand = "getlog";
  var editActionCommand = "editlog";
  casper.start('http://0.0.0.0:8000/ajax_dashboard');
  casper.then(function() {util.login(test);});
  casper.then(function() {
    util.moveToServersPage(test);
    casper.then(function() {
      util.registerMonitoringServer(test, {
        serverType: "8e632c14-d1f7-11e4-8350-d43d7e3146fb",
        baseURL: "http://127.0.0.1/zabbix/api_jsonrpc.php",
        brokerURL: "amqp://test_user:test_password@localhost:5673/test",
        nickName: "test",
        serverName: "test",
        userName: "admin",
        userPassword: "zabbix-admin"
      });
    });
  });
  // move to actions page
  util.openSettingMenu(test);
  casper.waitForSelector(x("//a[normalize-space(text())='アクション']"),
    function success() {
      test.assertExists(x("//a[normalize-space(text())='アクション']",
                          "Found 'actions' in menu"));
      this.click(x("//a[normalize-space(text())='アクション']"));
    },
    function fail() {
      test.assertExists(x("//a[normalize-space(text())='アクション']"));
    });
  casper.then(function() {
    casper.waitForUrl(/.*ajax_actions/, function() {
      test.assertUrlMatch(/.*ajax_actions/, "Move into actions page.");
    });
  });

  // create actions
  casper.waitForSelector("form button#add-action-button",
    function success() {
      test.assertExists("form button#add-action-button",
                        "Found add action button item");
      this.click("form button#add-action-button");
    },
    function fail() {
      test.assertExists("form button#add-action-button");
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
                        "Confirmation dialog button appeared when " +
                        "registering target action.");
      this.evaluate(function() {
        $("div.ui-dialog-buttonset").attr("area-described-by","server-selector")
          .last().find("button").click().change();
      });
    },
    function fail() {
      test.assertExists(".ui-dialog-buttonset > button");
    });
  casper.waitForSelector("input#inputActionCommand",
    function success() {
      this.sendKeys("input#inputActionCommand", actionCommand);
    },
    function fail() {
      test.assertExists("input#inputActionCommand");
    });
  casper.waitForSelector(".ui-dialog-buttonset > button",
    function success() {
      test.assertExists(".ui-dialog-buttonset > button",
                        "Confirmation dialog button appeared when " +
                        "registering action.");
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
  // check DOMNodeInserted event
  casper.waitFor(function() {
    return this.evaluate(function() {
      return $(document).on("DOMNodeInserted",  "table tr",
                            function() {return true;});
    });
  }, function then() {
    test.assertTextExists(actionCommand,
                          "Registered actionCommand: \"" +actionCommand+
                          "\" exists in the user role table.");
    this.evaluate(function() {
      $(document).off("DOMNodeInserted",  "table tr");
    });
  }, function timeout() {
    this.echo("Oops, table element does not to be newly created.");
  });
  // edit action
  casper.waitFor(function() {
    return this.evaluate(function() {
      return $("input.btn").last().click();
    });
  }, function then() {
    test.assertFieldCSS("input#inputActionCommand", actionCommand,
                        "Registered actionCommand: \"" + actionCommand +
                        "\" exists in the edit action dialog input#inputActionCommand.");
  }, function timeout() {
    this.echo("Oops, edit action dialog is not opened.");
  });
  casper.waitForSelector("input#inputActionCommand",
    function success() {
      this.sendKeys("input#inputActionCommand", editActionCommand, {reset: true});
    },
    function fail() {
      test.assertExists("input#inputActionCommand");
    });
  casper.waitForSelector(".ui-dialog-buttonset > button",
    function success() {
      test.assertExists(".ui-dialog-buttonset > button",
                        "Confirmation dialog button appeared when " +
                        "registering action.");
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
  // assert edit action command
  casper.waitFor(function() {
    return this.evaluate(function() {
      return $(document).on("DOMSubtreeModified", "table tr",
                            function() {return true;});
    });
  }, function then() {
    test.assertTextExists(editActionCommand,
                          "Edited action command \"" + editActionCommand +
                          "\" text exists.");
    this.evaluate(function() {
      $(document).off("DOMSubtreeModified", "table tr");
    });
  }, function timeout() {
    this.echo("Oops, it seems not to be logged in.");
  });
  casper.then(function() {
    this.evaluate(function() {
      $("tr:last").find(".selectcheckbox").click();
      return true;
    });
  });
  casper.waitForSelector("form button#delete-action-button",
    function success() {
      test.assertExists("form button#delete-action-button",
                        "Found delete action button.");
      this.click("form button#delete-action-button");
    },
    function fail() {
      test.assertExists("form button#delete-action-button");
    });

  // click Yes in delete action dialog
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
  casper.waitFor(function() {
    return this.evaluate(function() {
      return $(document).on("DOMNodeRemoved", "table tr",
                            function() {return true;});
    });
  }, function then() {
    test.assertTextDoesntExist(editActionCommand,
                               "Registered editActionCommand: \""
                               + editActionCommand +
                               "\" does not exist in the user role table.");
    this.evaluate(function() {
      $(document).off("DOMNodeRemoved", "table tr");
    });
  }, function timeout() {
    this.echo("Oops, newly created table element does not to be deleted.");
  });
  casper.then(function() {
    util.moveToServersPage(test);
    util.unregisterMonitoringServer();
  });
  casper.then(function() {util.logout(test);});
  casper.run(function() {test.done();});
});
