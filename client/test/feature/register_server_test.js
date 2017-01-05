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
})

casper.test.begin('Register/Unregister server test', function(test) {
  var server = {
        serverType: "8e632c14-d1f7-11e4-8350-d43d7e3146fb",
        baseURL: "http://127.0.0.1/zabbix/api_jsonrpc.php",
        nickName: "test",
        serverName: "testhost",
        brokerURL: "amqp://test_user:test_password@localhost:5673/test",
        userName: "admin",
        userPassword: "zabbix-admin"
  };
  var editedServer = {
        serverType: "8e632c14-d1f7-11e4-8350-d43d7e3146fb",
        baseURL: "http://127.0.0.1/zabbix/api_jsonrpc.php",
        nickName: "edited-test",
        serverName: "editedhost",
        brokerURL: "amqp://test_user:test_password@localhost:5673/test",
        userName: "edited-admin",
        userPassword: "edited-zabbix-admin"
  };
  casper.start('http://0.0.0.0:8000/ajax_dashboard');
  casper.then(function() {util.login(test);});
  // move to servers page
  util.openSettingMenu(test);
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
      this.sendKeys("input#server-edit-dialog-param-form-1", server.baseURL);
    },
    function fail() {
      test.assertExists("input#server-edit-dialog-param-form-1");
    });
  casper.waitForSelector("input#server-edit-dialog-param-form-2",
    function success() {
      this.sendKeys("input#server-edit-dialog-param-form-2", server.userName);
    },
    function fail() {
      test.assertExists("input#server-edit-dialog-param-form-2");
    });
  casper.waitForSelector("input#server-edit-dialog-param-form-3",
    function success() {
      this.sendKeys("input#server-edit-dialog-param-form-3", server.userPassword);
    },
    function fail() {
      test.assertExists("input#server-edit-dialog-param-form-3");
    });
  casper.waitForSelector("input#server-edit-dialog-param-form-7",
    function success() {
      this.sendKeys("input#server-edit-dialog-param-form-7", server.brokerURL);
    },
    function fail() {
      test.assertExists("input#server-edit-dialog-param-form-7");
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
  // Wait for the response and the addition of the registered server.
  casper.waitFor(function() {
    return this.evaluate(function() {
      return $("tr").length >= 2;
    });
  }, function then() {
    test.assertTextExists(server.nickName,
                          "Registered server's nickName \""
                          + server.nickName +
                          "\" exists in the monitoring servers table.");
  }, function timeout() {
    this.fail("Oops, confirmation dialog dose not to be closed.");
  });
  // edit monitoring server
  casper.waitFor(function() {
    return this.evaluate(function() {
      return $("input.btn").last().click();
    });
  }, function then() {
    test.assertFieldCSS("input#server-edit-dialog-param-form-0", server.nickName,
                        "Registered server's nickName: \""
                        + server.nickName +
                        "\" exists in the edit monitoring server dialog"+
                        " input#server-edit-dialog-param-form-0.");
  }, function timeout() {
    this.fail("Oops, edit monitoring server dialog is not opened.");
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
  // Wait for the response and the addition of the registered server.
  casper.waitForText(editedServer.nickName, function then() {
    test.assertTextExists(editedServer.nickName,
                          "Edited monitoring server's nickName \"" +
                          editedServer.nickName + "\" text exists.");
  }, function timeout() {
    this.fail("Oops, it seems not to be logged in.");
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
      return $("tr").length == 1;
    });
  }, function then() {
    test.assertTextDoesntExist(editedServer.nickName,
                               "Registered server's nickName \""
                               + editedServer.nickName +
                               "\" does not exist in the monitoring servers table.");
  }, function timeout() {
    this.fail("Oops, newly created table element does not to be deleted.");
  });
  casper.then(function() {util.logout(test);});
  casper.run(function() {test.done();});
});
