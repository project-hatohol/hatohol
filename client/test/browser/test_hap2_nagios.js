describe('hap2_nagios', function() {
  function getPlugin() {
    return hatohol["hap_902d955c-d1f7-11e4-80f9-d43d7e3146fb"]
  }

  it('getTopURL', function() {
    var plugin = getPlugin();
    var expected = "http://www.example.com/nagios/";
    var server = {
      "baseURL": expected,
    };
    expect(plugin.getTopURL(server)).to.be(expected);
  });

  it('getTopURL without baseURL', function() {
    var plugin = getPlugin();
    var server = {
    };
    expect(plugin.getTopURL(server)).to.be(undefined);
  });
});

