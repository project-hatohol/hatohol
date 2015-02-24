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
  casper.then(function() {util.moveIncidentSettingsPage(test);});
  casper.then(function() {
    util.registerIncidentTrackerRedmine(test,
                                        {nickName: "testmine1",
                                         ipAddress: "127.0.0.1",
                                         projectId: 1,
                                         apiKey: "testredmine"});
  });
  casper.then(function() {util.unregisterIncidentTrackerRedmine(test);});
  casper.then(function() {util.logout(test);});
  casper.run(function() {test.done();});
});
