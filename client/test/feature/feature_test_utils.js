function login(test) {
  casper.waitForSelector("input#inputUserName",
    function success() {
      this.sendKeys("input#inputUserName", "admin");
    });
  casper.waitForSelector("input#inputPassword",
    function success() {
      this.sendKeys("input#inputPassword", "hatohol");
    },
    function fail() {
      test.assertExists("input#inputPassword");
    });
  casper.then(function() {
    this.click('input#loginFormSubmit');
  });
};
exports.login = login;

function logout(test) {
  casper.then(function() {
    this.click('#userProfileButton');
    this.waitForSelector('#logoutMenuItem', function() {
      this.click('#logoutMenuItem');
    });
  });
};
exports.logout = logout;

function moveServersPage(test) {
  // move to servers page
  casper.waitForSelector(x("//a[normalize-space(text())='監視サーバー']"),
    function success() {
      test.assertExists(x("//a[normalize-space(text())='監視サーバー']",
                          "Found 'monitoring servers' menu"));
      this.click(x("//a[normalize-space(text())='監視サーバー']"));
    },
    function fail() {
      test.assertExists(x("//a[normalize-space(text())='監視サーバー']"));
    });
  casper.then(function() {
    casper.wait(200, function() {
      test.assertUrlMatch(/.*ajax_servers/, "Move into servers page.");
    });
  });
}
exports.moveServersPage = moveServersPage;

function registerMonitoringServer(test, params) {
  // prepare and show monitoring server adding dialog
  casper.waitForSelector("form button#add-server-button",
    function success() {
      this.click("form button#add-server-button");
    },
    function fail() {
      test.assertExists("form button#add-server-button");
    });
  casper.waitForSelector("select#selectServerType",
    function success() {
      this.evaluate(function(type) {
        $("select#selectServerType").val(type).change();
        return true;
      }, params.serverType);
    },
    function fail() {
      test.assertExists("select#selectServerType");
    });

  // emulate actual inputs
  casper.waitForSelector("input#server-edit-dialog-param-form-0",
    function success() {
      this.sendKeys("input#server-edit-dialog-param-form-0", params.nickName);
    },
    function fail() {
      test.assertExists("input#server-edit-dialog-param-form-0");
    });
  casper.waitForSelector("input#server-edit-dialog-param-form-1",
    function success() {
      this.sendKeys("input#server-edit-dialog-param-form-1", params.serverName);
    },
    function fail() {
      test.assertExists("input#server-edit-dialog-param-form-1");
    });
  casper.waitForSelector("input#server-edit-dialog-param-form-2",
    function success() {
      this.sendKeys("input#server-edit-dialog-param-form-2", params.ipAddress);
    },
    function fail() {
      test.assertExists("input#server-edit-dialog-param-form-2");
    });
  casper.waitForSelector("input#server-edit-dialog-param-form-4",
    function success() {
      this.sendKeys("input#server-edit-dialog-param-form-4", params.userName);
    },
    function fail() {
      test.assertExists("input#server-edit-dialog-param-form-4");
    });
  casper.waitForSelector("input#server-edit-dialog-param-form-5",
    function success() {
      this.sendKeys("input#server-edit-dialog-param-form-5", params.userPassword);
    },
    function fail() {
      test.assertExists("input#server-edit-dialog-param-form-5");
    });

  // close post confirmation dialog
  casper.waitForSelector(".ui-dialog-buttonset > button",
    function success() {
      this.click(".ui-dialog-buttonset > button");
    },
    function fail() {
      test.assertExists(".ui-dialog-buttonset > button");
    });
  // close result confirmation dialog
  casper.waitForSelector("div.ui-dialog-buttonset > button",
    function success() {
      this.click("div.ui-dialog-buttonset > button");
    },
    function fail() {
      test.assertExists("div.ui-dialog-buttonset > button");
    });
}
exports.registerMonitoringServer = registerMonitoringServer;

function unregisterMonitoringServer(test) {
  // check delete-selector check box in minitoring server
  casper.then(function() {
    casper.wait(200, function() {
      this.evaluate(function() {
        $("tr:last").find(".selectcheckbox").click();
        return true;
      });
    });
  });

  casper.waitForSelector("form button#delete-server-button",
    function success() {
      casper.capture('unregister.png');
      this.click("form button#delete-server-button");
    },
    function fail() {
      test.assertExists("form button#delete-server-button");
    });
  casper.waitForSelector("div.ui-dialog-buttonset button",
    function success() {
      this.evaluate(function() {
        $("div.ui-dialog-buttonset button").next().click();
      });
    },
    function fail() {
      test.assertExists("form button#delete-server-button");
    });
}
exports.unregisterMonitoringServer = unregisterMonitoringServer;
