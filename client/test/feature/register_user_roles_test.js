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
  var roleName = "watcher";
  var editedRoleName = "modifiedWatcher";
  casper.start('http://0.0.0.0:8000/ajax_dashboard');
  casper.then(function() {util.login(test);});
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
      this.sendKeys("input#editUserRoleName", roleName);
    },
    function fail() {
      test.assertExists("input#editUserRoleName");
    });
  // getting all users
  casper.waitForSelector("input#privilegeFlagId3",
    function success() {
      test.assertExists("input#privilegeFlagId3",
                        "Add getting all users privilege.");
      this.click("input#privilegeFlagId3");
    },
    function fail() {
      test.assertExists("input#privilegeFlagId3");
    });
  // getting all monitoring servers
  casper.waitForSelector("input#privilegeFlagId9",
    function success() {
      test.assertExists("input#privilegeFlagId9",
                        "Add getting all monitoring servers privilege.");
      this.click("input#privilegeFlagId9");
    },
    function fail() {
      test.assertExists("input#privilegeFlagId9");
    });
  // getting all actions
  casper.waitForSelector("input#privilegeFlagId15",
    function success() {
      test.assertExists("input#privilegeFlagId15",
                        "Add getting all actions privilege.");
      this.click("input#privilegeFlagId15");
    },
    function fail() {
      test.assertExists("input#privilegeFlagId15");
    });
  // getting all incident settings
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
  // check DOMNodeInserted event
  casper.waitFor(function() {
    return this.evaluate(function() {
      return $(document).on("DOMNodeInserted", "table#userRoleEditorMainTable tr",
                            function() {return true;});
    });
  }, function then() {
    test.assertTextExists(roleName,
                          "Registered user role's name \""
                          + roleName +
                          "\" exists in the user role table.");
    this.evaluate(function() {
      $(document).off("DOMNodeInserted", "table#userRoleEditorMainTable tr");
    });
  }, function timeout() {
    this.echo("Oops, table element does not to be newly created.");
  });

  // edit user role
  casper.waitForSelector("form button#edit-user-roles-button",
    function success() {
      test.assertExists("form button#edit-user-roles-button");
      this.click("form button#edit-user-roles-button");

    },
    function fail() {
      test.assertExists("form button#edit-user-roles-button");
    });
  casper.waitForSelector("form input[type=button][value='表示 / 編集'].editUserRole",
    function success() {
      test.assertExists("form input[type=button][value='表示 / 編集'].editUserRole");
      this.click("form input[type=button][value='表示 / 編集'].editUserRole");
    },
    function fail() {
      test.assertExists("form input[type=button][value='表示 / 編集']");
    });
  casper.waitForSelector("input#editUserRoleName",
    function success() {
      this.sendKeys("input#editUserRoleName", editedRoleName);
    },
    function fail() {
      test.assertExists("input#editUserRoleName");
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
  // assert edit user's name
  casper.waitFor(function() {
    return this.evaluate(function() {
      return $(document).on("DOMSubtreeModified", "table#userRoleEditorMainTable tr",
                            function() {return true;});
    });
  }, function then() {
    test.assertTextExists(editedRoleName,
                          "Edited role's name \"" + editedRoleName +
                          "\" text exists.");
    this.evaluate(function() {
      $(document).off("DOMSubtreeModified", "table#userRoleEditorMainTable tr");
    });
  }, function timeout() {
    this.echo("Oops, it seems not to be logged in.");
  });
  casper.then(function() {
    this.evaluate(function() {
      $("input.userRoleSelectCheckbox:last").click();
      return true;
    });
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
  casper.waitFor(function() {
    return this.evaluate(function() {
      return $(document).on("DOMNodeRemoved", "table#userRoleEditorMainTable tr",
                            function() {return true;});
    });
  }, function then() {
    test.assertTextDoesntExist(editedRoleName,
                               "Registered user role's name \""
                               + editedRoleName +
                               "\" does not exist in the user table.");
    this.evaluate(function() {
      $(document).off("DOMNodeRemoved", "table#userRoleEditorMainTable tr");
    });
  }, function timeout() {
    this.echo("Oops, newly created table element does not to be deleted.");
  });
  casper.then(function() {util.logout(test);});
  casper.run(function() {test.done();});
});
