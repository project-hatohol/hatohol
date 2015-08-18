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

casper.test.begin('Dashboard test', function(test) {
  casper.start('http://0.0.0.0:8000/ajax_dashboard');
  casper.then(function() {util.login(test);});
  casper.waitFor(function() {
    return this.evaluate(function() {
      return $(document).on("DOMSubtreeModified", "div#update-time",
                            function() {return true;});
    });
  }, function then() {
    casper.then(function() {
      test.assertTitle('ダッシュボード - Hatohol', 'It should match dashboard title.');
      test.assertTextExist('ダッシュボード', 'It should appear dashboard text.');
      test.assertTextExist('グローバルステータス',
                           'It should appear dashboard global status text.');
      test.assertTextExist('システムステータス',
                           'It should appear dashboard system status text.');
      test.assertTextExist('ホストステータス',
                           'It should appear dashboard host status text.');
      this.evaluate(function() {
        $(document).off("DOMSubtreeModified", "div#update-time");
      });
    });
  }, function timeout() {
    this.echo("Oops, it seems not to be logged in.");
  });
  casper.then(function() {util.logout(test);});
  casper.run(function() {test.done();});
});
