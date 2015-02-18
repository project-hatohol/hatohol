var x = require('casper').selectXPath;
var util = require('feature_test_utils');
casper.options.viewportSize = {width: 1024, height: 768};
casper.on('page.error', function(msg, trace) {
   this.echo('Error: ' + msg, 'ERROR');
   for(var i=0; i<trace.length; i++) {
       var step = trace[i];
       this.echo('   ' + step.file + ' (line ' + step.line + ')', 'ERROR');
   }
});

casper.test.begin('Dashboard test', function(test) {
  casper.start('http://0.0.0.0:8000/ajax_dashboard');
  casper.then(function() {util.login(test);});
  casper.then(function() {
    casper.wait(200, function() {
      casper.log('should appear after 200ms', 'info');
      test.assertTitle('ダッシュボード - Hatohol', 'should match dashboard title.');
      test.assertTextExist('ダッシュボード', 'should appear dashboard text.');
      test.assertTextExist('グローバルステータス',
                           'should appear dashboard global status text.');
      test.assertTextExist('システムステータス',
                           'should appear dashboard system status text.');
      test.assertTextExist('ホストステータス',
                           'should appear dashboard host status text.');
    });
  });
  casper.then(function() {util.logout(test);});
  casper.run(function() {test.done();});
});
