describe('Utils', function() {

describe('getServerLocation', function() {
  it('with valid zabbix server', function() {
    var server = {
      "type": hatohol.MONITORING_SYSTEM_ZABBIX,
      "ipAddress": "127.0.0.1",
      "name": "localhost",
      "port": 80
    };
    var expected = "http://127.0.0.1/zabbix/";
    expect(getServerLocation(server)).to.be(expected);
  });

  it('zabbix server with port', function() {
    var server = {
      "type": hatohol.MONITORING_SYSTEM_ZABBIX,
      "ipAddress": "127.0.0.1",
      "name": "localhost",
      "port": 8080
    };
    var expected = "http://127.0.0.1:8080/zabbix/";
    expect(getServerLocation(server)).to.be(expected);
  });

  it('ipv6 zabbix server', function() {
    var server = {
      "type": hatohol.MONITORING_SYSTEM_ZABBIX,
      "ipAddress": "::1",
      "name": "localhost",
      "port": 8080
    };
    var expected = "http://[::1]:8080/zabbix/";
    expect(getServerLocation(server)).to.be(expected);
  });

  it('with valid nagios server', function() {
    var server = {
      "type": hatohol.MONITORING_SYSTEM_NAGIOS,
      "ipAddress": "127.0.0.1",
      "name": "localhost"
    };
    var expected = "http://127.0.0.1/nagios/";
    expect(getServerLocation(server)).to.be(expected);
  });

  it('nagios server with port', function() {
    var server = {
      "type": hatohol.MONITORING_SYSTEM_NAGIOS,
      "ipAddress": "127.0.0.1",
      "name": "localhost",
      "port": 8080
    };
    var expected = "http://127.0.0.1:8080/nagios/";
    expect(getServerLocation(server)).to.be(expected);
  });

  it('ipv6 nagios server', function() {
    var server = {
      "type": hatohol.MONITORING_SYSTEM_NAGIOS,
      "ipAddress": "::1",
      "name": "localhost"
    };
    var expected = "http://[::1]/nagios/";
    expect(getServerLocation(server)).to.be(expected);
  });

  it('with unknown server type', function() {
    var server = {
      "type": hatohol.MONITORING_SYSTEM_UNKNOWN,
      "ipAddress": "192.168.19.111",
      "NAME": "localhost"
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
      "type": hatohol.MONITORING_SYSTEM_ZABBIX,
      "ipAddress": "192.168.23.119",
      "name": "localhost"
    };
    var expected = "http://192.168.23.119/zabbix/maps.php";
    expect(getMapsLocation(server)).to.be(expected);
  });

  it('with valid nagios server', function() {
    var server = {
      "type": hatohol.MONITORING_SYSTEM_NAGIOS,
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

describe('getServerName', function() {
  it('with valid name', function() {
      var server = { "name": "server" };
      var serverId = 2;
      expect(getServerName(server, serverId)).to.be("server");
  });

  it('with no server', function() {
      var serverId = 2;
      var expected = gettext("Unknown") + " (ID: " + serverId + ")";
      expect(getServerName(undefined, serverId)).to.be(expected);
  });

  it('with no name', function() {
      var serverId = 2;
      var expected = gettext("Unknown") + " (ID: " + serverId + ")";
      expect(getServerName({}, serverId)).to.be(expected);
  });
});

describe('getHostName', function() {
  it('with valid name', function() {
      var server = {
	  "name": "server",
	  "hosts": {
	      "2": {
		  "name": "host"
	      }
	  }
      };
      var id = 2;
      expect(getHostName(server, id)).to.be("host");
  });

  it('with no server', function() {
      var id = 2;
      var expected = gettext("Unknown") + " (ID: " + id + ")";
      expect(getHostName(undefined, id)).to.be(expected);
  });

  it('with no host', function() {
      var id = 2;
      var expected = gettext("Unknown") + " (ID: " + id + ")";
      var server = { "name": "server" };
      expect(getHostName(server, id)).to.be(expected);
  });
});

describe('getHostgroupName', function() {
  it('with valid name', function() {
      var server = {
	  "name": "server",
	  "groups": {
	      "2": {
		  "name": "hostgroup"
	      }
	  }
      };
      var id = 2;
      expect(getHostgroupName(server, id)).to.be("hostgroup");
  });

  it('with no server', function() {
      var id = 2;
      var expected = gettext("Unknown") + " (ID: " + id + ")";
      expect(getHostgroupName(undefined, id)).to.be(expected);
  });

  it('with no hostgroup', function() {
      var id = 2;
      var expected = gettext("Unknown") + " (ID: " + id + ")";
      var server = { "name": "server" };
      expect(getHostgroupName(server, id)).to.be(expected);
  });
});

describe('getTriggerBrief', function() {
  it('with valid name', function() {
      var server = {
	  "name": "server",
	  "triggers": {
	      "2": {
		  "name": "trigger"
	      }
	  }
      };
      var id = 2;
      expect(getTriggerBrief(server, id)).to.be("trigger");
  });

  it('with no server', function() {
      var id = 2;
      var expected = gettext("Unknown") + " (ID: " + id + ")";
      expect(getTriggerBrief(undefined, id)).to.be(expected);
  });

  it('with no trigger', function() {
      var id = 2;
      var expected = gettext("Unknown") + " (ID: " + id + ")";
      var server = { "name": "server" };
      expect(getTriggerBrief(server, id)).to.be(expected);
  });
});

});
