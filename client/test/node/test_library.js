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

describe('getMapsLocation', function() {
  it('getMapsLocation with valid zabbix server', function() {
    var server = {
      "type": 0,
      "ipAddress": "192.168.23.119",
      "name": "localhost"
    };
    var expected = "http://192.168.23.119/zabbix/maps.php"
    expect(Library.getMapsLocation(server)).to.be(expected);
  });

  it('getMapsLocation with valid nagios server', function() {
    var server = {
      "type": 1,
      "ipAddress": "192.168.22.118",
      "name": "localhost"
    };
    expect(Library.getMapsLocation(server)).to.be(undefined);
  });

  it('getMapsLocation with unknown server type', function() {
    var server = {
      "type": 2,
      "ipAddress": "192.168.19.111",
      "name": "localhost"
    };
    expect(Library.getMapsLocation(server)).to.be(undefined);
  });
});
