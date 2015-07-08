describe('Utils', function() {

describe('isFloat', function() {
  it('0 (number)', function() {
    expect(isFloat(0)).to.be(false);
  });

  it('0.0 (number)', function() {
    // It's treated as "0".
    // It's a limitation of JavaScript.
    expect(isFloat(0.0)).to.be(false);
  });

  it('0.0 (string)', function() {
    expect(isFloat("0.0")).to.be(true);
  });

  it('1 (number)', function() {
    // It's treated as "1".
    // It's a limitation of JavaScript.
    expect(isFloat(1)).to.be(false);
  });

  it('1.0 (number)', function() {
    expect(isFloat(1.0)).to.be(false);
  });

  it('1.0 (string)', function() {
    expect(isFloat("1.0")).to.be(true);
  });
});

describe('isInt', function() {
  it('1', function() {
    expect(isInt(1)).to.be(true);
  });

  it('1.1', function() {
    expect(isInt(1.1)).to.be(false);
  });
});

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
    // issue #839
    //var expected = "http://127.0.0.1/nagios/";
    var expected = undefined;
    expect(getServerLocation(server)).to.be(expected);
  });

  it('nagios server with port', function() {
    var server = {
      "type": hatohol.MONITORING_SYSTEM_NAGIOS,
      "ipAddress": "127.0.0.1",
      "name": "localhost",
      "port": 8080
    };
    // issue #839
    //var expected = "http://127.0.0.1:8080/nagios/";
    var expected = undefined;
    expect(getServerLocation(server)).to.be(expected);
  });

  it('nagios server with baseURL', function() {
    var server = {
      "type": hatohol.MONITORING_SYSTEM_NAGIOS,
      "ipAddress": "127.0.0.1",
      "name": "localhost",
      "port": 8080,
      "baseURL": "http://127.0.0.1/nagios3/"
    };
    var expected = "http://127.0.0.1/nagios3/";
    expect(getServerLocation(server)).to.be(expected);
  });

  it('ipv6 nagios server', function() {
    var server = {
      "type": hatohol.MONITORING_SYSTEM_NAGIOS,
      "ipAddress": "::1",
      "name": "localhost"
    };
    // issue #839
    //var expected = "http://[::1]/nagios/";
    var expected = undefined;
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
    var server = {
	"type": hatohol.MONITORING_SYSTEM_ZABBIX
    };
    var expected = "Zabbix";
    expect(makeMonitoringSystemTypeLabel(server)).to.be(expected);
  });

  it('with valid nagios server', function() {
    var server = {
	"type": hatohol.MONITORING_SYSTEM_NAGIOS
    };
    var expected = "Nagios";
    expect(makeMonitoringSystemTypeLabel(server)).to.be(expected);
  });

  it('with valid HAPI zabbix server', function() {
    var server = {
	"type": hatohol.MONITORING_SYSTEM_HAPI_ZABBIX
    };
    var expected = "Zabbix (HAPI)";
    expect(makeMonitoringSystemTypeLabel(server)).to.be(expected);
  });

  it('with valid HAPI JSON', function() {
    var server = {
	"type": hatohol.MONITORING_SYSTEM_HAPI_JSON
    };
    var expected = "General Plugin";
    expect(makeMonitoringSystemTypeLabel(server)).to.be(expected);
  });

  it('with valid HAPI CEILOMETER', function() {
    var server = {
	"type": hatohol.MONITORING_SYSTEM_HAPI_CEILOMETER
    };
    var expected = "Ceilometer";
    expect(makeMonitoringSystemTypeLabel(server)).to.be(expected);
  });

  it('with unknown server type', function() {
    var server = {
	"type": hatohol.MONITORING_SYSTEM_UNKNOWN
    };
    var expected = "Invalid: " + server.type;
    expect(makeMonitoringSystemTypeLabel(server)).to.be(expected);
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

  it('plugin dependent', function() {
    var server = {
      "name": "server",
      "uuid": "f0166fb0-26ea-4566-abf4-077d1584e4f4",
      "type": hatohol.MONITORING_SYSTEM_HAPI2,
    }
    hatohol["hap_f0166fb0-26ea-4566-abf4-077d1584e4f4"] = {
      "getHostName": function(server, hostId) { return hostId.toLowerCase() },
    }
    var id = "My ID"
    expect(getHostName(server, id)).to.be("my id");
  });
});

describe('getNickName', function() {
  it('with valid nickname', function() {
    var server = {
      "name": "server",
      "nickname": "hostWithNickName",
      "hosts": {
        "2": {
          "name": "host"
        }
      }
    };
    var id = 2;
    expect(getNickName(server, id)).to.be("hostWithNickName");
  });

  it('with no server', function() {
    var id = 2;
    var expected = gettext("Unknown") + " (ID: " + id + ")";
    expect(getNickName(undefined, id)).to.be(expected);
  });

  it('with no nickname', function() {
    var id = 2;
    var expected = gettext("Unknown") + " (ID: " + id + ")";
    var server = { "name": "server" };
    expect(getNickName(server, id)).to.be(expected);
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
      'limit': '',
      'offset': '20',
    };
    expect(deparam(query)).eql(expected);
  });

  it('empty key', function() {
    var query = "=10&offset=20";
    var expected = {
      '': '10',
      'offset': '20',
    };
    expect(deparam(query)).eql(expected);
  });

  it('zero as key', function() {
    var query = "0=value";
    var expected = {
      0: "value"
    };
    expect(deparam(query)).eql(expected);
  });

  it('decode', function() {
    var query = "f%5B%5D=status&f%5B%5D=severity&offset=%3E20";
    var expected = {
      'f': ['status', 'severity'],
      'offset': '>20',
    };
    expect(deparam(query)).eql(expected);
  });

  it('complex object', function() {
    var expected = {
      items: [
        {
          serverId: 1,
          hostId: 2,
          itemId: 3,
          beginTime: 4,
          endTime: 5
        },
        {
          serverId: 6,
          hostId: 7,
          itemId: 8,
          beginTime: 9,
          endTime: 10
        },
      ]
    };
    var query = $.param(expected);
    var actual = deparam(query);
    expect(actual).eql(expected);
  });
});

describe('formatItemValue', function() {
  it('Text', function() {
    expect(formatItemValue("Host12")).eql("Host12");
  });

  it('Integer without unit', function() {
    expect(formatItemValue("87937923434")).eql("87937923434");
  });

  it('Integer with unit', function() {
    expect(formatItemValue("87937923434", 'bps')).eql("87.94 Gbps");
  });

  it('Integer without metric prefix', function() {
    expect(formatItemValue("999", 'bps')).eql("999 bps");
  });

  it('Integer with metric prefix', function() {
    expect(formatItemValue("1000", 'bps')).eql("1 Kbps");
  });

  it('Float without unit', function() {
    expect(formatItemValue("0.982348234")).eql(0.9823);
  });

  it('Float with trailing 0', function() {
    expect(formatItemValue('0.15000', '')).eql("0.15");
    expect(formatItemValue('0.0015000', '')).eql("0.0015");
  });

  it('Large float', function() {
    expect(formatItemValue('123456789.98765', '')).eql("1.235e+8");
  });

  it('Percent', function() {
    expect(formatItemValue("1000.9234", '%')).eql("1001 %");
  });

  it('uptime less than one day', function() {
    var seconds = "" + (60 * 60 * 24 - 1);
    expect(formatItemValue(seconds, 'uptime')).eql("23:59:59");
  });

  it('uptime with one day', function() {
    var seconds = "" + (60 * 60 * 24 + 1);
    expect(formatItemValue(seconds, 'uptime')).eql("1 day, 00:00:01");
  });

  it('uptime with two days', function() {
    var seconds = "" + (60 * 60 * 24 * 2 + 1);
    expect(formatItemValue(seconds, 'uptime')).eql("2 days, 00:00:01");
  });

  it('Kilo Bytes', function() {
    expect(formatItemValue('2049', 'B')).eql("2.001 KB");
  });

  it('Mega Bytes', function() {
    var bytes = "" + (2.5184 * 1024 * 1024);
    expect(formatItemValue(bytes, 'B')).eql("2.518 MB");
  });

  it('Giga Bytes', function() {
    var bytes = "" + (2.5189 * 1024 * 1024 * 1024);
    expect(formatItemValue(bytes, 'B')).eql("2.519 GB");
  });
});

describe('plugin', function() {
  var uuid = "6820b219-fe13-4b54-b9ac-1ab8e851c017";
  var server = {
    "type": hatohol.MONITORING_SYSTEM_HAPI2,
    "uuid": uuid
  };

  beforeEach(function() {
    hatohol["hap_" + uuid] = {
      type: uuid,
      label: "TestPlugin"
    };
  });

  afterEach(function() {
    delete hatohol["hap_" + uuid];
  });

  it('getServerLocation', function() {
    var expected = "http://127.0.0.1/test-plugin/";
    hatohol["hap_" + uuid] = {
      getTopURL: function(server) {
	return expected;
      }
    };
    expect(getServerLocation(server)).to.be(expected);
  });

  it('getServerLocation without plugin function', function() {
    expect(getServerLocation(server)).to.be(undefined);
  });

  it('getMapsLocation', function() {
    var expected = "http://127.0.0.1/test-plugin/maps/";
    hatohol["hap_" + uuid] = {
      getMapsURL: function(server) {
	return expected;
      }
    };
    expect(getMapsLocation(server)).to.be(expected);
  });

  it('getMapsLocation without plugin function', function() {
    expect(getMapsLocation(server)).to.be(undefined);
  });

  it('getItemGraphLocation', function() {
    var expected = "http://127.0.0.1/test-plugin/item/graph/";
    var itemId = 1;
    hatohol["hap_" + uuid] = {
      getItemGraphURL: function(server, itemId) {
        return expected;
      }
    };
    expect(getItemGraphLocation(server)).to.be(expected);
  });

  it('getItemGraphLocation without plugin function', function() {
    var itemId = 1;
    expect(getItemGraphLocation(server, itemId)).to.be(undefined);
  });

  it('makeMonitoringSystemTypeLabel', function() {
    var expected = "TestPlugin";
    expect(makeMonitoringSystemTypeLabel(server)).to.be(expected);
  });
});

});
