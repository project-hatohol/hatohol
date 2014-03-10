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

  it('get status label at OK state', function() {
    checkGetStatusLabel(hatohol.ARM_WORK_STAT_OK, "OK", "text-success");
  });

  it('get status label at Error state', function() {
    checkGetStatusLabel(hatohol.ARM_WORK_STAT_FAILURE, "Error", "text-error");
  });

  it('get status label at unknown state', function() {
    checkGetStatusLabel(12345, "Unknown:12345", "text-error");
  });

  it('get info HTML before calling setServerId', function() {
    var pkt = {serverConnStat:{'5':{}}};
    var parser = new ServerConnStatParser(pkt);
    var thrown = false;
    try {
      parser.getInfoHTML();
    } catch (e) {
      thrown = true;
      expect(e).to.be.a(Error);
    } finally {
      expect(thrown).to.be(true);
    }
  });

  it('get info HTML', function() {
    var pkt = {serverConnStat:
      {
        '5':{
          status: hatohol.ARM_WORK_STAT_OK,
          running: 1,
          statUpdateTime: "1394444393.469501123",
          lastSuccessTime: "1394444393.469501123",
          lastFailureTime: "0.000000000",
          numUpdate: 100,
          numFailure: 5,
          failureComment: "Bouno",
        },
      }
    };
    var parser = new ServerConnStatParser(pkt);
    expect(parser.setServerId(5)).to.be(true); 
    var html = parser.getInfoHTML();
    var expectStr =
      gettext("Running") + ": " + gettext("Yes") + "<br>" +
      gettext("Status update time") + ": " +
        gettext("2014-03-10 18:39:53") + "<br>" +
      gettext("Last success time") + ": " +
        gettext("2014-03-10 18:39:53") + "<br>" + 
      gettext("Last failure time") + ": " + gettext("-") + "<br>" +
      gettext("Number of communication") + ": 100" + "<br>" +
      gettext("Number of failure") + ": 5" + "<br>" +
      gettext("Comment for the failure") + ": Bouno";
    expect(html).to.be(expectStr);
  });

});
