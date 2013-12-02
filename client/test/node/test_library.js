var expect = require('expect.js');
var Library = require("../../static/js/library");

describe('getServerLocation', function() {
  it('with valid zabbix server', function() {
    var server = {
      "type": 0,
      "ipAddress": "127.0.0.1",
      "name": "localhost"
    };
    var expected = "http://127.0.0.1/zabbix/";
    expect(Library.getServerLocation(server)).to.be(expected);
  });

  it('with valid nagios server', function() {
    var server = {
      "type": 1,
      "ipAddress": "127.0.0.1",
      "name": "localhost"
    };
    expect(Library.getServerLocation(server)).to.be(undefined);
  });
});

describe('getItemGraphLocation', function() {
  it('getItemGrapLocation with valid zabbix server', function() {
    var server = {
      "type": 0,
      "ipAddress": "127.0.0.1",
      "name": "localhost"
    };
    var itemId = 1129;
    var expected =
      "http://127.0.0.1/zabbix/history.php?action=showgraph&amp;itemid=1129"
    expect(Library.getItemGraphLocation(server, itemId)).to.be(expected);
  });

  it('getItemGraphLocation with valid nagios server', function() {
    var server = {
      "type": 1,
      "ipAddress": "127.0.0.1",
      "name": "localhost"
    };
    var itemId = 1129;
    expect(Library.getItemGraphLocation(server, itemId)).to.be(undefined);
  });
});
