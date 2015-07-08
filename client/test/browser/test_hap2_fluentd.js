describe('hap2_fluentd', function() {
  function getPlugin() {
    return hatohol["hap_d91ba1cb-a64a-4072-b2b8-2f91bcae1818"]
  }

  it('getHostName', function() {
    expect(getPlugin().getHostName("", "HOGE")).to.be("HOGE")
  });
});

