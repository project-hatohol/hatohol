describe('hap2_zabbix', function() {
  var server = {
    "baseURL": "http://www.example.com/zabbix/api_jsonrpc.php",
  };

  function getPlugin() {
    return hatohol["hap_8e632c14-d1f7-11e4-8350-d43d7e3146fb"]
  }

  it('getTopURL', function() {
    var plugin = getPlugin();
    var expected = "http://www.example.com/zabbix/";
    expect(plugin.getTopURL(server)).to.be(expected);
  });

  it('getTopURL with unknown suffix', function() {
    var plugin = getPlugin();
    var brokenServer = {
      "baseURL": "http://www.example.com/zabbix/api_xmlrpc.php",
    };
    expect(plugin.getTopURL(brokenServer)).to.be(undefined);
  });

  it('getMapsURL', function() {
    var plugin = getPlugin();
    var expected = "http://www.example.com/zabbix/maps.php";
    expect(plugin.getMapsURL(server)).to.be(expected);
  });

  it('getItemGraphURL', function() {
    var plugin = getPlugin();
    var expected = "http://www.example.com/zabbix/history.php?action=showgraph&itemid=911&itemids%5B%5D=911";
    expect(plugin.getItemGraphURL(server, 911)).to.be(expected);
  });
});

