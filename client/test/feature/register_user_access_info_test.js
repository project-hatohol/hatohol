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

casper.test.begin('Register/Unregister access info list test', function(test) {
  var monitoringServer = {
        serverType: "8e632c14-d1f7-11e4-8350-d43d7e3146fb",
        baseURL: "http://127.0.0.1/zabbix/api_jsonrpc.php",
        brokerURL: "amqp://test_user:test_password@localhost:5673/test",
        nickName: "zabbix",
        serverName: "test-zabbix",
        ipAddress: "127.0.0.1",
        userName: "admin",
        userPassword: "zabbix-admin",
  };
  casper.start('http://0.0.0.0:8000/ajax_dashboard');
  casper.then(function() {util.login(test);});
  casper.then(function() {
    util.moveToServersPage(test);
    casper.then(function() {
      util.registerMonitoringServer(test, monitoringServer);
    });
  });
  util.openSettingMenu(test);
  casper.waitForSelector(x("//a[normalize-space(text())='ユーザー']"),
    function success() {
      test.assertExists(x("//a[normalize-space(text())='ユーザー']"));
      this.click(x("//a[normalize-space(text())='ユーザー']"));
    },
    function fail() {
      test.assertExists(x("//a[normalize-space(text())='ユーザー']"));
    });
  casper.then(function() {
    casper.waitForUrl(/.*ajax_users/, function() {
      test.assertUrlMatch(/.*ajax_users/, "Move into users page.");
    });
  });
  // start to register access info list server
  casper.waitForSelector("form input[type=button][value='表示 / 編集']",
    function success() {
      test.assertExists("form input[type=button][value='表示 / 編集']");
      this.click("form input[type=button][value='表示 / 編集']");
    },
    function fail() {
      test.assertExists("form input[type=button][value='表示 / 編集']");
    });
  // register access info list server
  casper.waitForSelector("#privilegeEditDialogMainTable tr .serverSelectCheckbox",
    function success() {
      test.assertExists("#privilegeEditDialogMainTable tr .serverSelectCheckbox");
      this.evaluate(function() {
        $("#privilegeEditDialogMainTable tr:last").find(".serverSelectCheckbox")
          .click();
        return true;
      });
    },
    function fail() {
      this.evaluate(function() {
        $("#privilegeEditDialogMainTable tr:last").find(".serverSelectCheckbox");
        return true;
      });
    });
  // assert access info list server (Registered)
  casper.waitFor(function() {
     return this.evaluate(function () {
       return $("#privilegeEditDialogMainTable tr:last")
         .find(".serverSelectCheckbox:checked").val() !== undefined;
     });
  }, function then() {
     this.test.assert(this.evaluate(function () {
       return $("#privilegeEditDialogMainTable tr:last")
         .find(".serverSelectCheckbox:checked").val() !== undefined;
     }), "#privilegeEditDialogMainTable tr:last .serverSelectCheckbox is checked.");
  }, function timeout() {
    this.echo("Oops, #privilegeEditDialogMainTable .serverSelectCheckbox dose not to be checked.");
  });
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
  // start to unregister access info list server
  casper.waitForSelector("form input[type=button][value='表示 / 編集']",
    function success() {
      test.assertExists("form input[type=button][value='表示 / 編集']");
      this.click("form input[type=button][value='表示 / 編集']");
    },
    function fail() {
      test.assertExists("form input[type=button][value='表示 / 編集']");
    });
  // unregister access info list server
  casper.waitForSelector("#privilegeEditDialogMainTable tr .serverSelectCheckbox",
    function success() {
      test.assertExists("#privilegeEditDialogMainTable tr .serverSelectCheckbox");
      this.evaluate(function() {
        $("#privilegeEditDialogMainTable tr:last").find(".serverSelectCheckbox")
          .click();
        return true;
      });
    },
    function fail() {
      this.evaluate(function() {
        $("#privilegeEditDialogMainTable tr:last").find(".serverSelectCheckbox");
        return true;
      });
    });
  // assert access info list server (Unregistered)
  casper.waitFor(function() {
    return this.evaluate(function () {
      return $("#privilegeEditDialogMainTable tr:last")
        .find(".serverSelectCheckbox:checked").val() === undefined;
    });
  }, function then() {
     this.test.assert(this.evaluate(function () {
       return $("#privilegeEditDialogMainTable tr:last")
         .find(".serverSelectCheckbox:checked").val() === undefined;
     }), "#privilegeEditDialogMainTable tr:last .serverSelectCheckbox is unchecked.");
  }, function timeout() {
    this.echo("Oops, #privilegeEditDialogMainTable .serverSelectCheckbox dose not to be unchecked.");
  });
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
  casper.then(function() {
    util.moveToServersPage(test);
    util.unregisterMonitoringServer();
  });
  casper.then(function() {util.logout(test);});
  casper.run(function() {test.done();});
});
