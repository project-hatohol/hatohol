describe('ServerView', function() {

  it('pass an undefined packet', function() {
    var parser = new ServerConnStatParser();
    expect(parser.isBadPacket()).to.be(true); 
  });
});
