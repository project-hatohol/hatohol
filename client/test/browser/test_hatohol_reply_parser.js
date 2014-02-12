describe('HatoholReplyParser', function() {
  beforeEach(function() {

  });

  afterEach(function() {
  });

  it('new with HTERR_OK', function() {
    var reply = {
      apiVersion: 3,
      errorCode: hatohol.HTERR_OK
    };
    var parser = new HatoholReplyParser(reply);
    expect(parser.getStatus()).to.be(REPLY_STATUS.OK);
  });
});
