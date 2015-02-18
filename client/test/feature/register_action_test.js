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

casper.test.begin('Register/Unregister action test', function(test) {
  casper.start('http://0.0.0.0:8000/ajax_dashboard');
  casper.then(function() {util.login(test);});
  casper.then(function() {
    util.moveServersPage(test);
    casper.then(function() {
      util.registerMonitoringServer(test,
                                    {serverType: 0,
                                     nickName: "test",
                                     serverName: "test",
                                     ipAddress: "127.0.0.1",
                                     userName: "admin",
                                     userPassword: "zabbix-admin"});
    });
  });
  // move to actions page
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
    casper.wait(200, function() {
      test.assertUrlMatch(/.*ajax_actions/, "Move into actions page.");
    });
  });
  casper.then(function() {
    util.moveServersPage(test);
    util.unregisterMonitoringServer();
  });
  casper.then(function() {util.logout(test);});
  casper.run(function() {test.done();});
});
