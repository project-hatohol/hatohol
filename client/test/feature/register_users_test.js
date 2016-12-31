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

casper.test.begin('Register/Unregister user test', function(test) {
  var user = {name: "testuser1",
              password: "testuser"};
  var editedUser = {name: "edituser1",
                    password: "edituser"};
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
  // create user
  casper.waitForSelector("form button#add-user-button",
    function success() {
      test.assertExists("form button#add-user-button",
                        "Found add user button item.");
      this.click("form button#add-user-button");
    },
    function fail() {
      test.assertExists("form button#add-user-button");
    });
  casper.waitForSelector("input#editUserName",
    function success() {
      test.assertExists("input#editUserName");
      this.click("input#editUserName");
    },
    function fail() {
      test.assertExists("input#editUserName");
    });
  casper.waitForSelector("input#editUserName",
    function success() {
      this.sendKeys("input#editUserName", user.name);
    },
    function fail() {
      test.assertExists("input#editUserName");
    });
  casper.waitForSelector("input#editPassword",
    function success() {
      this.sendKeys("input#editPassword", user.password);
    },
    function fail() {
      test.assertExists("input#editPassword");
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
  casper.waitFor(function() {
    return this.evaluate(function() {
      return $(document).on("DOMNodeInserted",  "table tr",
                            function() {return true;});
    });
  }, function then() {
    test.assertTextExists(user.name,
                          "Registered user's name \"" + user.name +
                          "\" exists in the user table.");
    this.evaluate(function() {
      $(document).off("DOMNodeInserted",  "table tr");
    });
  }, function timeout() {
    this.echo("Oops, table element does not to be newly created.");
  });

  casper.then(function() {
    this.evaluate(function() {
      return $("table tr:last").find("a").click();
    });
  });
  // edit user
  casper.waitFor(function() {
    return this.evaluate(function() {
      return $(document).on("DOMSubtreeModified",  "input#editUserName",
                            function() {return true;});
    });
  }, function then() {
    test.assertFieldCSS("input#editUserName", user.name,
                        "Registered user's name: \"" + user.name +
                        "\" exists in the last registered user's name in " +
                        "dialog input#editUserName.");
    this.evaluate(function() {
      $(document).off("DOMSubtreeModified",  "input#editUserName");
    });
  }, function timeout() {
    this.echo("Oops, edit user dialog is not opened.");
  });
  casper.waitForSelector("input#editUserName",
    function success() {
      this.sendKeys("input#editUserName", editedUser.name, {reset: true});
    },
    function fail() {
      test.assertExists("input#editUserName");
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
      return $(document).on("DOMSubtreeModified", "table tr",
                            function() {return true;});
    });
  }, function then() {
    test.assertTextExists(editedUser.name,
                          "Edited user's name \"" + editedUser.name +
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
  casper.waitForSelector("form button#delete-user-button",
    function success() {
      test.assertExists("form button#delete-user-button",
                        "Found delete user button.");
      this.click("form button#delete-user-button");
    },
    function fail() {
      test.assertExists("form button#delete-user-button");
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
      return $(document).on("DOMNodeRemoved", "table tr",
                            function() {return true;});
    });
  }, function then() {
    test.assertTextDoesntExist(editedUser.name,
                               "Registered user's name \""
                               + editedUser.name +
                               "\" does not exist in the user table.");
    this.evaluate(function() {
      $(document).off("DOMNodeRemoved", "table tr");
    });
  }, function timeout() {
    this.echo("Oops, newly created table element does not to be deleted.");
  });
  casper.then(function() {util.logout(test);});
  casper.run(function() {test.done();});
});
