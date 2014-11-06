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

  it('with valid HAPI zabbix server', function() {
    var server = {
      "type": hatohol.MONITORING_SYSTEM_HAPI_ZABBIX,
      "ipAddress": "127.0.0.1",
      "name": "localhost",
      "port": 80
    };
    var expected = "http://127.0.0.1/zabbix/";
    expect(getServerLocation(server)).to.be(expected);
  });

  it('HAPI zabbix server with port', function() {
    var server = {
      "type": hatohol.MONITORING_SYSTEM_HAPI_ZABBIX,
      "ipAddress": "127.0.0.1",
      "name": "localhost",
      "port": 8080
    };
    var expected = "http://127.0.0.1:8080/zabbix/";
    expect(getServerLocation(server)).to.be(expected);
  });

  it('ipv6 HAPI zabbix server', function() {
    var server = {
      "type": hatohol.MONITORING_SYSTEM_HAPI_ZABBIX,
      "ipAddress": "::1",
      "name": "localhost",
      "port": 8080
    };
    var expected = "http://[::1]:8080/zabbix/";
    expect(getServerLocation(server)).to.be(expected);
  });

  it('with valid HAPI nagios server', function() {
    var server = {
      "type": hatohol.MONITORING_SYSTEM_HAPI_NAGIOS,
      "ipAddress": "127.0.0.1",
      "name": "localhost"
    };
    var expected = "http://127.0.0.1/nagios/";
    expect(getServerLocation(server)).to.be(expected);
  });

  it('HAPI nagios server with port', function() {
    var server = {
      "type": hatohol.MONITORING_SYSTEM_HAPI_NAGIOS,
      "ipAddress": "127.0.0.1",
      "name": "localhost",
      "port": 8080
    };
    var expected = "http://127.0.0.1:8080/nagios/";
    expect(getServerLocation(server)).to.be(expected);
  });

  it('ipv6 HAPI nagios server', function() {
    var server = {
      "type": hatohol.MONITORING_SYSTEM_HAPI_NAGIOS,
      "ipAddress": "::1",
      "name": "localhost"
    };
    var expected = "http://[::1]/nagios/";
    expect(getServerLocation(server)).to.be(expected);
  });

  it('with HAPI JSON type(unknown)', function() {
    var server = {
      "type": hatohol.MONITORING_SYSTEM_HAPI_JSON,
      "ipAddress": "192.168.19.111",
      "NAME": "localhost"
    };
    expect(getServerLocation(server)).to.be(undefined);
  });

  it('with HAPI CEILOMETER type(unknown)', function() {
    var server = {
      "type": hatohol.MONITORING_SYSTEM_HAPI_CEILOMETER,
      "ipAddress": "192.168.19.111",
      "NAME": "localhost"
    };
    expect(getServerLocation(server)).to.be(undefined);
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
      "type": hatohol.MONITORING_SYSTEM_ZABBIX,
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
      "type": hatohol.MONITORING_SYSTEM_NAGIOS,
      "ipAddress": "127.0.0.1",
      "name": "localhost"
    };
    var itemId = 1129;
    expect(getItemGraphLocation(server, itemId)).to.be(undefined);
  });

  it('with valid HAPI zabbix server', function() {
    var server = {
      "type": hatohol.MONITORING_SYSTEM_HAPI_ZABBIX,
      "ipAddress": "127.0.0.1",
      "name": "localhost"
    };
    var itemId = 1129;
    var expected =
      "http://127.0.0.1/zabbix/history.php?action=showgraph&amp;itemid=1129";
    expect(getItemGraphLocation(server, itemId)).to.be(expected);
  });

  it('with valid HAPI nagios server', function() {
    var server = {
      "type": hatohol.MONITORING_SYSTEM_HAPI_NAGIOS,
      "ipAddress": "127.0.0.1",
      "name": "localhost"
    };
    var itemId = 1129;
    expect(getItemGraphLocation(server, itemId)).to.be(undefined);
  });

  it('with valid HAPI JSON', function() {
    var server = {
      "type": hatohol.MONITORING_SYSTEM_HAPI_JSON,
      "ipAddress": "127.0.0.1",
      "name": "localhost"
    };
    var itemId = 1129;
    expect(getItemGraphLocation(server, itemId)).to.be(undefined);
  });

  it('with valid HAPI CEILOMETER', function() {
    var server = {
      "type": hatohol.MONITORING_SYSTEM_HAPI_CEILOMETER,
      "ipAddress": "127.0.0.1",
      "name": "localhost"
    };
    var itemId = 1129;
    expect(getItemGraphLocation(server, itemId)).to.be(undefined);
  });

  it('with unknown server type', function() {
    var server = {
      "type": hatohol.MONITORING_SYSTEM_UNKNOWN,
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

  it('with valid HAPI zabbix server', function() {
    var server = {
      "type": hatohol.MONITORING_SYSTEM_HAPI_ZABBIX,
      "ipAddress": "192.168.23.119",
      "name": "localhost"
    };
    var expected = "http://192.168.23.119/zabbix/maps.php";
    expect(getMapsLocation(server)).to.be(expected);
  });

  it('with valid HAPI nagios server', function() {
    var server = {
      "type": hatohol.MONITORING_SYSTEM_HAPI_NAGIOS,
      "ipAddress": "192.168.22.118",
      "name": "localhost"
    };
    expect(getMapsLocation(server)).to.be(undefined);
  });

  it('with valid HAPI JSON(unknown)', function() {
    var server = {
      "type": hatohol.MONITORING_SYSTEM_HAPI_NAGIOS,
      "ipAddress": "192.168.22.118",
      "name": "localhost"
    };
    expect(getMapsLocation(server)).to.be(undefined);
  });

  it('with valid HAPI CEILOMETER(unknown)', function() {
    var server = {
      "type": hatohol.MONITORING_SYSTEM_HAPI_NAGIOS,
      "ipAddress": "192.168.22.118",
      "name": "localhost"
    };
    expect(getMapsLocation(server)).to.be(undefined);
  });

  it('with unknown server type', function() {
    var server = {
      "type": hatohol.MONITORING_SYSTEM_UNKNOWN,
      "ipAddress": "192.168.19.111",
      "name": "localhost"
    };
    expect(getMapsLocation(server)).to.be(undefined);
  });
});

describe('makeMonitoringSystemTypeLabel', function() {
  it('with valid zabbix server', function() {
    var type = hatohol.MONITORING_SYSTEM_ZABBIX;
    var expected = "ZABBIX";
    expect(makeMonitoringSystemTypeLabel(type)).to.be(expected);
  });

  it('with valid nagios server', function() {
    var type = hatohol.MONITORING_SYSTEM_NAGIOS;
    var expected = "NAGIOS";
    expect(makeMonitoringSystemTypeLabel(type)).to.be(expected);
  });

  it('with valid HAPI zabbix server', function() {
    var type = hatohol.MONITORING_SYSTEM_HAPI_ZABBIX;
    var expected = "ZABBIX(HAPI)";
    expect(makeMonitoringSystemTypeLabel(type)).to.be(expected);
  });

  it('with valid HAPI nagios server', function() {
    var type = hatohol.MONITORING_SYSTEM_HAPI_NAGIOS;
    var expected = "NAGIOS(HAPI)";
    expect(makeMonitoringSystemTypeLabel(type)).to.be(expected);
  });

  it('with valid HAPI JSON', function() {
    var type = hatohol.MONITORING_SYSTEM_HAPI_JSON;
    var expected = "GENERAL PLUGIN";
    expect(makeMonitoringSystemTypeLabel(type)).to.be(expected);
  });

  it('with valid HAPI CEILOMETER', function() {
    var type = hatohol.MONITORING_SYSTEM_HAPI_CEILOMETER;
    var expected = "CEILOMETER";
    expect(makeMonitoringSystemTypeLabel(type)).to.be(expected);
  });

  it('with unknown server type', function() {
    var type = hatohol.MONITORING_SYSTEM_UNKNOWN;
    var expected = "INVALID: " + type;
    expect(makeMonitoringSystemTypeLabel(type)).to.be(expected);
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
	  "brief": "trigger"
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

describe('deparam', function() {
  it('1 param', function() {
    var query = "limit=10";
    var expected = {
      limit: '10',
    };
    expect(deparam(query)).eql(expected);
  });

  it('empty value', function() {
    var query = "limit=&offset=20";
    var expected = {
      'limit': undefined,
      'offset': '20',
    };
    expect(deparam(query)).eql(expected);
  });

  it('empty key', function() {
    var query = "=10&offset=20";
    var expected = {
      'offset': '20',
    };
    expect(deparam(query)).eql(expected);
  });

  it('decode', function() {
    var query = "f%5B%5D=status&offset=%3E20";
    var expected = {
      'f[]': 'status',
      'offset': '>20',
    };
    expect(deparam(query)).eql(expected);
  });
});

describe('valueString', function() {
  it('Text', function() {
    expect(valueString("Host12")).eql("Host12");
  });

  it('Integer without unit', function() {
    expect(valueString("87937923434")).eql("87937923434");
  });

  it('Integer with unit', function() {
    expect(valueString("87937923434", 'bps')).eql("87.94 Gbps");
  });

  it('Integer without metric prefix', function() {
    expect(valueString("999", 'bps')).eql("999 bps");
  });

  it('Integer with metric prefix', function() {
    expect(valueString("1000", 'bps')).eql("1.000 Kbps");
  });

  it('Float without unit', function() {
    expect(valueString("0.982348234")).eql(0.9823);
  });

  it('Percent', function() {
    expect(valueString("1000.9234", '%')).eql("1001 %");
  });

  it('uptime less than one day', function() {
    var seconds = "" + (60 * 60 * 24 - 1);
    expect(valueString(seconds, 'uptime')).eql("23:59:59");
  });

  it('uptime with one day', function() {
    var seconds = "" + (60 * 60 * 24 + 1);
    expect(valueString(seconds, 'uptime')).eql("1 day, 00:00:01");
  });

  it('uptime with two days', function() {
    var seconds = "" + (60 * 60 * 24 * 2 + 1);
    expect(valueString(seconds, 'uptime')).eql("2 days, 00:00:01");
  });

  it('Kilo Bytes', function() {
    expect(valueString('2049', 'B')).eql("2.001 KB");
  });

  it('Mega Bytes', function() {
    var bytes = "" + (2.5184 * 1024 * 1024)
    expect(valueString(bytes, 'B')).eql("2.518 MB");
  });

  it('Giga Bytes', function() {
    var bytes = "" + (2.5189 * 1024 * 1024 * 1024)
    expect(valueString(bytes, 'B')).eql("2.519 GB");
  });
});

});
