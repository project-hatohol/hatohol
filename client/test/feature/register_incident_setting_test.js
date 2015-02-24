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

casper.test.begin('Register/Unregister incident settings test', function(test) {
  casper.start('http://0.0.0.0:8000/ajax_dashboard');
  casper.then(function() {util.login(test);});
  casper.then(function() {
    util.moveServersPage(test);
    casper.then(function() {
      // register Monitoring Server (Zabbix)
      util.registerMonitoringServer(test,
                                    {serverType: 0,
                                     nickName: "zabbix",
                                     serverName: "test-zabbix",
                                     ipAddress: "127.0.0.1",
                                     userName: "admin",
                                     userPassword: "zabbix-admin"});
    });
  });
  casper.then(function() {util.moveIncidentSettingsPage(test);});
  casper.then(function() {
    util.registerIncidentTrackerRedmine(test,
                                        {nickName: "testmine1",
                                         ipAddress: "http://127.0.0.1",
                                         projectId: "1",
                                         apiKey: "testredmine"});
  });
  casper.waitForSelector("form button#add-incident-setting-button",
    function success() {
      test.assertExists("form button#add-incident-setting-button");
      this.click("form button#add-incident-setting-button");
    },
    function fail() {
      test.assertExists("form button#add-incident-setting-button");
    });
  casper.then(function() {util.unregisterIncidentTrackerRedmine(test);});
  casper.then(function() {
    util.moveServersPage(test);
    util.unregisterMonitoringServer();
  });
  casper.then(function() {util.logout(test);});
  casper.run(function() {test.done();});
});
