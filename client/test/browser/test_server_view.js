describe('ServerView', function() {

  it('pass an undefined packet', function() {
    var parser = new ServerConnStatParser();
    expect(parser.isBadPacket()).to.be(true); 
  });

  it('pass an undefined serverConnStat', function() {
    var pkt = {};
    var parser = new ServerConnStatParser(pkt);
    expect(parser.isBadPacket()).to.be(true); 
  });
});
