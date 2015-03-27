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

casper.test.begin('Register/Unregister server test', function(test) {
  var server = {serverType: 0, // Zabbix
                nickName: "test",
                serverName: "testhost",
                ipAddress: "127.0.0.1",
                userName: "admin",
                userPassword: "zabbix-admin"};
  var editedServer = {serverType: 0, // Zabbix
                      nickName: "edited-test",
                      serverName: "editedhost",
                      ipAddress: "127.0.1.1",
                      userName: "edited-admin",
                      userPassword: "edited-zabbix-admin"};
  casper.start('http://0.0.0.0:8000/ajax_dashboard');
  casper.then(function() {util.login(test);});
  // move to servers page
  casper.waitForSelector(x("//a[normalize-space(text())='監視サーバー']"),
    function success() {
      test.assertExists(x("//a[normalize-space(text())='監視サーバー']",
                          "Found 'monitoring servers' in menu"));
      this.click(x("//a[normalize-space(text())='監視サーバー']"));
    },
    function fail() {
      test.assertExists(x("//a[normalize-space(text())='監視サーバー']"));
    });
  casper.then(function() {
    casper.waitForUrl(/.*ajax_servers/, function() {
      test.assertUrlMatch(/.*ajax_servers/, "Move into servers page.");
    });
  });

  // create monitoring server
  casper.waitForSelector("form button#add-server-button",
    function success() {
      test.assertExists("form button#add-server-button",
                        "Found add server button item");
      this.click("form button#add-server-button");
    },
    function fail() {
      test.assertExists("form button#add-server-button");
    });

  casper.waitForSelector("select#selectServerType",
    function success() {
      test.assertExists("select#selectServerType", "Found server type selector.");
      this.evaluate(function(type) {
        $('select#selectServerType').val(type).change();
        return true;
      }, server.serverType);
    },
    function fail() {
      test.assertExists("select#selectServerType");
    });
  casper.waitForSelector("input#server-edit-dialog-param-form-0",
    function success() {
      this.sendKeys("input#server-edit-dialog-param-form-0", server.nickName);
    },
    function fail() {
      test.assertExists("input#server-edit-dialog-param-form-0");
    });
  casper.waitForSelector("input#server-edit-dialog-param-form-1",
    function success() {
      this.sendKeys("input#server-edit-dialog-param-form-1", server.serverName);
    },
    function fail() {
      test.assertExists("input#server-edit-dialog-param-form-1");
    });
  casper.waitForSelector("input#server-edit-dialog-param-form-2",
    function success() {
      this.sendKeys("input#server-edit-dialog-param-form-2", server.ipAddress);
    },
    function fail() {
      test.assertExists("input#server-edit-dialog-param-form-2");
    });
  casper.waitForSelector("input#server-edit-dialog-param-form-4",
    function success() {
      this.sendKeys("input#server-edit-dialog-param-form-4", server.userName);
    },
    function fail() {
      test.assertExists("input#server-edit-dialog-param-form-4");
    });
  casper.waitForSelector("input#server-edit-dialog-param-form-5",
    function success() {
      this.sendKeys("input#server-edit-dialog-param-form-5", server.userPassword);
    },
    function fail() {
      test.assertExists("input#server-edit-dialog-param-form-5");
    });
  casper.waitForSelector(".ui-dialog-buttonset > button",
    function success() {
      test.assertExists(".ui-dialog-buttonset > button",
                        "Confirmation dialog button appeared when registering.");
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
      return $(document).on("DOMNodeInserted",
                            "table tr",
                            function() {return true;});
    });
  }, function then() {
    test.assertTextExists(server.nickName,
                          "Registered server's nickName \"" +server.nickName+
                          "\" exists in the monitoring servers table.");
    this.evaluate(function() {
      $(document).off("DOMNodeInserted", "table tr");
    });
  }, function timeout() {
    this.echo("Oops, confirmation dialog dose not to be closed.");
  });
  // edit monitoring server
  casper.waitFor(function() {
    return this.evaluate(function() {
      return $("input.btn").last().click();
    });
  }, function then() {
    test.assertFieldCSS("input#server-edit-dialog-param-form-0", server.nickName,
                        "Registered server's nickName: \"" +server.nickName+
                        "\" exists in the edit monitoring server dialog"+
                        " input#server-edit-dialog-param-form-0.");
  }, function timeout() {
    this.echo("Oops, edit monitoring server dialog is not opened.");
  });
  casper.waitForSelector("input#server-edit-dialog-param-form-0",
    function success() {
      this.sendKeys("input#server-edit-dialog-param-form-0",
                    editedServer.nickName, {reset: true});
    },
    function fail() {
      test.assertExists("input#server-edit-dialog-param-form-0");
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
    test.assertTextExists(editedServer.nickName,
                          "Edited monitoring server's nickName \"" +
                          editedServer.nickName + "\" text exists.");
    this.evaluate(function() {
      $(document).off("DOMSubtreeModified", "table tr");
    });
  }, function timeout() {
    this.echo("Oops, it seems not to be logged in.");
  });
  // start to delete monitoring server
  casper.then(function() {
    this.evaluate(function() {
      $("tr:last").find(".selectcheckbox").click();
      return true;
    });
  });
  casper.waitForSelector("form button#delete-server-button",
    function success() {
      test.assertExists("form button#delete-server-button",
                        "Found delete server button.");
      this.click("form button#delete-server-button");
    },
    function fail() {
      test.assertExists("form button#delete-server-button");
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
  casper.waitFor(function() {
    return this.evaluate(function() {
      return $(document).on("DOMNodeRemoved",
                            "table tr",
                            function() {return true;});
    });
  }, function then() {
    test.assertTextDoesntExist(editedServer.nickName,
                               "Registered server's nickName \"" +
                               editedServer.nickName +
                               "\" does not exists in the monitoring servers table.");
    this.evaluate(function() {
      $(document).off("DOMNodeRemoved", "table tr");
    });
  }, function timeout() {
    this.echo("Oops, newly created table element does not to be deleted.");
  });
  casper.then(function() {util.logout(test);});
  casper.run(function() {test.done();});
});
