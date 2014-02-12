describe('HatoholReplyParser', function() {
  beforeEach(function() {
  });

  afterEach(function() {
  });

  it('new with HTERR_OK', function() {
    var reply = {
      apiVersion: hatohol.FACE_REST_API_VERSION,
      errorCode: hatohol.HTERR_OK
    };
    var parser = new HatoholReplyParser(reply);
    expect(parser.getStatus()).to.be(REPLY_STATUS.OK);
  });

  it('null', function() {
    var parser = new HatoholReplyParser(null);
    expect(parser.getStatus()).to.be(REPLY_STATUS.NULL_OR_UNDEFINED);
  });

  it('undefined', function() {
    var parser = new HatoholReplyParser(undefined);
    expect(parser.getStatus()).to.be(REPLY_STATUS.NULL_OR_UNDEFINED);
  });

  it('no apiVersion', function() {
    var reply = {
      errorCode: hatohol.HTERR_OK
    };
    var parser = new HatoholReplyParser(reply);
    expect(parser.getStatus()).to.be(REPLY_STATUS.NOT_FOUND_API_VERSION);
  });

  it('unmatched apiVersion', function() {
    var reply = {
      apiVersion: hatohol.FACE_REST_API_VERSION + 1,
      errorCode: hatohol.HTERR_OK
    };
    var parser = new HatoholReplyParser(reply);
    expect(parser.getStatus()).to.be(REPLY_STATUS.UNSUPPORTED_API_VERSION);
  });
});
