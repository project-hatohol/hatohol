describe('hap2_ceilometer', function() {
  function getPlugin() {
    return hatohol["hap_aa25a332-d1f7-11e4-80b4-d43d7e3146fb"]
  }

  it('getHostName: N/A', function() {
    expect(getPlugin().getHostName("", "N/A")).to.be("N/A")
  });

  it('getHostName: normal strings', function() {
    expect(getPlugin().getHostName("", "normal")).to.be(null)
  });
});

