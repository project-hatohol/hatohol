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

casper.test.begin('Register/Unregister user role test', function(test) {
  casper.start('http://0.0.0.0:8000/ajax_dashboard');
  casper.then(function() {util.login(test);});
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
  // create user role
  casper.waitForSelector("form button#edit-user-roles-button",
    function success() {
      test.assertExists("form button#edit-user-roles-button");
      this.click("form button#edit-user-roles-button");
    },
    function fail() {
      test.assertExists("form button#edit-user-roles-button");
    });
  casper.waitForSelector("form input[type=button][value='追加']",
    function success() {
      test.assertExists("form input[type=button][value='追加']");
      this.click("form input[type=button][value='追加']");
    },
    function fail() {
      test.assertExists("form input[type=button][value='追加']");
    });
  casper.waitForSelector("input#editUserRoleName",
    function success() {
      this.sendKeys("input#editUserRoleName", "watcher");
    },
    function fail() {
      test.assertExists("input#editUserRoleName");
    });
  // 全てのユーザーの取得
  casper.waitForSelector("input#privilegeFlagId3",
    function success() {
      test.assertExists("input#privilegeFlagId3",
                        "Add getting all users privilege.");
      this.click("input#privilegeFlagId3");
    },
    function fail() {
      test.assertExists("input#privilegeFlagId3");
    });
  // 全ての監視サーバーの取得
  casper.waitForSelector("input#privilegeFlagId9",
    function success() {
      test.assertExists("input#privilegeFlagId9",
                        "Add getting all monitoring servers privilege.");
      this.click("input#privilegeFlagId9");
    },
    function fail() {
      test.assertExists("input#privilegeFlagId9");
    });
  // 全てのアクションの取得
  casper.waitForSelector("input#privilegeFlagId15",
    function success() {
      test.assertExists("input#privilegeFlagId15",
                        "Add getting all actions privilege.");
      this.click("input#privilegeFlagId15");
    },
    function fail() {
      test.assertExists("input#privilegeFlagId15");
    });
  // 全てのインシデント管理設定の取得
  casper.waitForSelector("input#privilegeFlagId22",
    function success() {
      test.assertExists("input#privilegeFlagId22",
                        "Add getting all incident settings privilege.");
      this.click("input#privilegeFlagId22");
    },
    function fail() {
      test.assertExists("input#privilegeFlagId22");
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
  // check delete-selector check box in user role
  casper.waitFor(function() {
    return this.evaluate(function() {
      return document.querySelectorAll("div.ui-dialog").length < 2;
    });
  }, function then() {
      this.evaluate(function() {
        $("input.userRoleSelectCheckbox:last").click();
        return true;
      });
  }, function timeout() {
    this.echo("Oops, confirmation dialog seems not to be closed.");
  });
  casper.waitForSelector("input#deleteUserRolesButton",
    function success() {
      test.assertExists("input#deleteUserRolesButton",
                        "Found delete user roles button.");
      this.click("input#deleteUserRolesButton");
    },
    function fail() {
      test.assertExists("input#deleteUserRolesButton");
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
      test.assertExists("form button#delete-user-button");
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
  casper.then(function() {util.logout(test);});
  casper.run(function() {test.done();});
});
