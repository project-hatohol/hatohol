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

function openSettingMenu(test) {
  casper.waitForSelector(x("//a[normalize-space(text())='設定']"),
    function success() {
      this.click(x("//a[normalize-space(text())='設定']"));
    });
}
exports.openSettingMenu = openSettingMenu;

function moveToServersPage(test) {
  // move to servers page
  openSettingMenu(test);
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
    casper.waitForUrl(/.*ajax_servers/, function() {
      test.assertUrlMatch(/.*ajax_servers/, "Move into servers page.");
    });
  });
}
exports.moveToServersPage = moveToServersPage;

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
  // TODO: These can be use only for Zabbix (HAPI2). We should support
  //       other types.
  casper.waitForSelector("input#server-edit-dialog-param-form-0",
    function success() {
      this.sendKeys("input#server-edit-dialog-param-form-0", params.nickName);
    },
    function fail() {
      test.assertExists("input#server-edit-dialog-param-form-0");
    });
  casper.waitForSelector("input#server-edit-dialog-param-form-1",
    function success() {
      this.sendKeys("input#server-edit-dialog-param-form-1", params.baseURL);
    },
    function fail() {
      test.assertExists("input#server-edit-dialog-param-form-1");
    });
  casper.waitForSelector("input#server-edit-dialog-param-form-2",
    function success() {
      this.sendKeys("input#server-edit-dialog-param-form-2", params.userName);
    },
    function fail() {
      test.assertExists("input#server-edit-dialog-param-form-2");
    });
  casper.waitForSelector("input#server-edit-dialog-param-form-3",
    function success() {
      this.sendKeys("input#server-edit-dialog-param-form-3", params.userPassword);
    },
    function fail() {
      test.assertExists("input#server-edit-dialog-param-form-3");
    });
  casper.waitForSelector("input#server-edit-dialog-param-form-7",
    function success() {
      this.sendKeys("input#server-edit-dialog-param-form-7", params.brokerURL);
    },
    function fail() {
      test.assertExists("input#server-edit-dialog-param-form-7");
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
  // check delete-selector checkbox in minitoring server
  casper.waitFor(function() {
    return this.evaluate(function() {
      return document.querySelectorAll("div.ui-dialog").length < 1;
    });
  }, function then() {
    this.evaluate(function() {
      $("tr:last").find(".selectcheckbox").click();
      return true;
    });
  }, function timeout() {
    this.echo("Oops, confirmation dialog dose not to be closed.");
  });

  casper.waitForSelector("form button#delete-server-button",
    function success() {
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

function moveToIncidentSettingsPage(test) {
  // move to incident setting page
  openSettingMenu(test);
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
}
exports.moveToIncidentSettingsPage = moveToIncidentSettingsPage;

function registerIncidentTrackerRedmine(test, params) {
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
           this.sendKeys("input#editIncidentTrackerNickname", params.nickName);
       },
       function fail() {
           test.assertExists("input#editIncidentTrackerNickname");
   });
   casper.waitForSelector("input#editIncidentTrackerBaseURL",
       function success() {
           this.sendKeys("input#editIncidentTrackerBaseURL", params.ipAddress);
       },
       function fail() {
           test.assertExists("input#editIncidentTrackerBaseURL");
   });
   casper.waitForSelector("input#editIncidentTrackerProjectId",
       function success() {
           this.sendKeys("input#editIncidentTrackerProjectId", params.projectId);
       },
       function fail() {
           test.assertExists("input#editIncidentTrackerProjectId");
   });
   casper.waitForSelector("input#editIncidentTrackerUserName",
       function success() {
           this.sendKeys("input#editIncidentTrackerUserName", params.apiKey);
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
  // close editing incident tracker dialog
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
}
exports.registerIncidentTrackerRedmine = registerIncidentTrackerRedmine;

function unregisterIncidentTrackerRedmine(test) {
  // open editing incident tracker dialog
  casper.waitForSelector("button#edit-incident-trackers-button",
    function success() {
      test.assertExists("button#edit-incident-trackers-button",
                        "Found adding incident trackers button item");
      this.click("button#edit-incident-trackers-button");
    },
    function fail() {
      test.assertExists("form button#add-incident-trackers-button");
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
    this.echo("Oops, confirmation dialog dose not to be closed.");
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
                        "Confirmation dialog appeared, and then close it.");
      this.evaluate(function() {
        $("div.ui-dialog-buttonset button").last().click();
      });
    },
    function fail() {
      test.assertExists("div.ui-dialog-buttonset button");
    });
}
exports.unregisterIncidentTrackerRedmine = unregisterIncidentTrackerRedmine;

function moveToDashboardPage(test) {
  // move to dashboard page
  casper.then(function () {
    this.click(x("//a[normalize-space(text())='ダッシュボード']"));
  });
  casper.then(function() {
    casper.waitForUrl(/.*ajax_dashboard/, function() {
      test.assertUrlMatch(/.*ajax_dashboard/, "Move into dashboard page.");
    });
  });
}
exports.moveToDashboardPage = moveToDashboardPage;

function moveToLogSearchSystemPage(test) {
  openSettingMenu(test);
  casper.waitForSelector(x("//a[normalize-space(text())='ログ検索システム']"),
    function success() {
      test.assertExists(x("//a[normalize-space(text())='ログ検索システム']"));
      this.click(x("//a[normalize-space(text())='ログ検索システム']"));
    },
    function fail() {
      test.assertExists(x("//a[normalize-space(text())='ログ検索システム']"));
    });
  casper.then(function() {
    casper.waitForUrl(/.*ajax_log_search_system/, function() {
      test.assertUrlMatch(/.*ajax_log_search_system/,
                          "Move into logsearch system page.");
    });
  });
}
exports.moveToLogSearchSystemPage = moveToLogSearchSystemPage;
