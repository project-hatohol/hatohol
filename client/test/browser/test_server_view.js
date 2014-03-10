describe('ServerView', function() {

  function checkGetStatusLabel(stat, expectMsg, expectMsgClass) {
    var pkt = {serverConnStat:{'5':{status:stat}}};
    var parser = new ServerConnStatParser(pkt);
    expect(parser.setServerId(5)).to.be(true); 
    var label = parser.getStatusLabel();
    expect(label.msg).to.be(expectMsg);
    expect(label.msgClass).to.be(expectMsgClass);
  }

  // -------------------------------------------------------------------------
  // Test cases
  // -------------------------------------------------------------------------
  it('pass an undefined packet', function() {
    var parser = new ServerConnStatParser();
    expect(parser.isBadPacket()).to.be(true); 
  });

  it('pass an undefined serverConnStat', function() {
    var pkt = {};
    var parser = new ServerConnStatParser(pkt);
    expect(parser.isBadPacket()).to.be(true); 
  });

  it('set nonexisting server id', function() {
    var pkt = {serverConnStat:{}};
    var parser = new ServerConnStatParser(pkt);
    expect(parser.setServerId(5)).to.be(false); 
  });

  it('set existing server id', function() {
    var pkt = {serverConnStat:{'5':{}}};
    var parser = new ServerConnStatParser(pkt);
    expect(parser.setServerId(5)).to.be(true); 
  });

  it('get status label before calling setServerId()', function() {
    var pkt = {serverConnStat:{'5':{}}};
    var parser = new ServerConnStatParser(pkt);
    var thrown = false;
    try {
      parser.getStatusLabel();
    } catch (e) {
      thrown = true;
      expect(e).to.be.a(Error);
    } finally {
      expect(thrown).to.be(true);
    }
  });

  it('get status label with no status member', function() {
    var pkt = {serverConnStat:{'5':{}}};
    var parser = new ServerConnStatParser(pkt);
    expect(parser.setServerId(5)).to.be(true); 
    expect(parser.getStatusLabel()).to.be("N/A");
  });

  it('get status label at initial state', function() {
    checkGetStatusLabel(hatohol.ARM_WORK_STAT_INIT,
                        "Initial State", "text-warning");
  });

});
