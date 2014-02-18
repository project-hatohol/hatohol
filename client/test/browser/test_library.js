describe('library.js', function() {

describe('getServerLocation', function() {
  it('with valid zabbix server', function() {
    var server = {
      "type": 0,
      "ipAddress": "127.0.0.1",
      "name": "localhost",
      "port": 80
    };
    var expected = "http://127.0.0.1/zabbix/";
    expect(getServerLocation(server)).to.be(expected);
  });

  it('zabbix server with port', function() {
    var server = {
      "type": 0,
      "ipAddress": "127.0.0.1",
      "name": "localhost",
      "port": 8080
    };
    var expected = "http://127.0.0.1:8080/zabbix/";
    expect(getServerLocation(server)).to.be(expected);
  });

  it('ipv6 zabbix server', function() {
    var server = {
      "type": 0,
      "ipAddress": "::1",
      "name": "localhost",
      "port": 8080
    };
    var expected = "http://[::1]:8080/zabbix/";
    expect(getServerLocation(server)).to.be(expected);
  });

  it('with valid nagios server', function() {
    var server = {
      "type": 1,
      "ipAddress": "127.0.0.1",
      "name": "localhost"
    };
    expect(getServerLocation(server)).to.be(undefined);
  });
});

describe('getItemGraphLocation', function() {
  it('with valid zabbix server', function() {
    var server = {
      "type": 0,
      "ipAddress": "127.0.0.1",
      "name": "localhost"
    };
    var itemId = 1129;
    var expected =
      "http://127.0.0.1/zabbix/history.php?action=showgraph&amp;itemid=1129";
    expect(getItemGraphLocation(server, itemId)).to.be(expected);
  });

  it('with valid nagios server', function() {
    var server = {
      "type": 1,
      "ipAddress": "127.0.0.1",
      "name": "localhost"
    };
    var itemId = 1129;
    expect(getItemGraphLocation(server, itemId)).to.be(undefined);
  });
});

describe('getMapsLocation', function() {
  it('with valid zabbix server', function() {
    var server = {
      "type": 0,
      "ipAddress": "192.168.23.119",
      "name": "localhost"
    };
    var expected = "http://192.168.23.119/zabbix/maps.php";
    expect(getMapsLocation(server)).to.be(expected);
  });

  it('with valid nagios server', function() {
    var server = {
      "type": 1,
      "ipAddress": "192.168.22.118",
      "name": "localhost"
    };
    expect(getMapsLocation(server)).to.be(undefined);
  });

  it('with unknown server type', function() {
    var server = {
      "type": 2,
      "ipAddress": "192.168.19.111",
      "name": "localhost"
    };
    expect(getMapsLocation(server)).to.be(undefined);
  });
});

});
