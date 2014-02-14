describe("HatoholReplyParser", function() {
  it("null", function() {
    var parser = new HatoholReplyParser(null);
    var stat = parser.getStatus();
    expect(stat).to.be(REPLY_STATUS.NULL_OR_UNDEFINED);
  });

  it("undefined", function() {
    var parser = new HatoholReplyParser(undefined);
    var stat = parser.getStatus();
    expect(stat).to.be(REPLY_STATUS.NULL_OR_UNDEFINED);
  });

  it("not found apiVersion", function() {
    var reply = {"errorCode":0};
    var parser = new HatoholReplyParser(reply);
    var stat = parser.getStatus();
    expect(stat).to.be(REPLY_STATUS.NOT_FOUND_API_VERSION);
  });

  it("not support API version", function() {
    var reply = {"apiVersion":1};
    var parser = new HatoholReplyParser(reply);
    var stat = parser.getStatus();
    expect(stat).to.be(REPLY_STATUS.UNSUPPORTED_API_VERSION);
  });

  it("not found errorCode", function() {
    var reply = {"apiVersion":hatohol.FACE_REST_API_VERSION};
    var parser = new HatoholReplyParser(reply);
    var stat = parser.getStatus();
    var message = parser.getMessage();
    var expectedMessage = gettext("Not found errorCode.");
    expect(stat).to.be(REPLY_STATUS.NOT_FOUND_ERROR_CODE);
    expect(message).to.be(expectedMessage);
  });

  it("errorCode is not OK", function() {
    var reply = {"apiVersion":hatohol.FACE_REST_API_VERSION,
                 errorCode:hatohol.HTERR_UNKOWN_REASON};
    var parser = new HatoholReplyParser(reply);
    var stat = parser.getStatus();
    expect(stat).to.be(REPLY_STATUS.ERROR_CODE_IS_NOT_OK);
  });

  it("get error code", function() {
    var reply = {"apiVersion":hatohol.FACE_REST_API_VERSION,
                 "errorCode":hatohol.HTERR_ERROR_TEST};
    var parser = new HatoholReplyParser(reply);
    var stat = parser.getStatus();
    var errorCode = parser.getErrorCode();
    expect(stat).to.be(REPLY_STATUS.ERROR_CODE_IS_NOT_OK);
    expect(errorCode).to.be(hatohol.HTERR_ERROR_TEST);
  });

  it("get error code name", function() {
    var reply = {"apiVersion":hatohol.FACE_REST_API_VERSION,
                 "errorCode":hatohol.HTERR_ERROR_TEST};
    var parser = new HatoholReplyParser(reply);
    var errorCodeName = parser.getErrorCodeName();
    expect(errorCodeName).to.be("HTERR_ERROR_TEST");
  });

  it("use a pre-defined message", function() {
    var reply = {
      "apiVersion":hatohol.FACE_REST_API_VERSION,
      "errorCode":hatohol.HTERR_ERROR_TEST,
      "errorMessage":"server's error message"
    };
    var parser = new HatoholReplyParser(reply);
    var message = parser.getMessage();
    expect(message).to.be("Error test.");
  });

  it("use a server's message", function() {
    var reply = {
      "apiVersion":hatohol.FACE_REST_API_VERSION,
      "errorCode":hatohol.HTERR_ERROR_TEST_WITHOUT_MESSAGE,
      "errorMessage":"server's error message"
    };
    var parser = new HatoholReplyParser(reply);
    var message = parser.getMessage();
    expect(message).to.be(reply["errorMessage"]);
  });

  it("without a message", function() {
    var reply = {
      "apiVersion":hatohol.FACE_REST_API_VERSION,
      "errorCode":hatohol.HTERR_ERROR_TEST_WITHOUT_MESSAGE
    };
    var parser = new HatoholReplyParser(reply);
    var message = parser.getMessage();
    var expected = "Error: " + hatohol.HTERR_ERROR_TEST_WITHOUT_MESSAGE +
      ", " + "HTERR_ERROR_TEST_WITHOUT_MESSAGE";
    expect(message).to.be(expected);
  });

  it("use a option message", function() {
    var reply = {
      "apiVersion":hatohol.FACE_REST_API_VERSION,
      "errorCode":hatohol.HTERR_ERROR_TEST,
      "errorMessage":"server's error message",
      "optionMessages":"option message",
    };
    var parser = new HatoholReplyParser(reply);
    var message = parser.getMessage();
    expect(message).to.be("Error test.");
    expect(parser.optionMessages).to.be(reply.optionMessages);
  });
});

describe("HatoholLoginReplyParser", function() {
  it("no sessionId", function() {
    var reply = {
      "apiVersion": hatohol.FACE_REST_API_VERSION,
      "errorCode": hatohol.HTERR_OK
    };
    var parser = new HatoholLoginReplyParser(reply);
    var stat = parser.getStatus();
    expect(stat).to.be(REPLY_STATUS.NOT_FOUND_SESSION_ID);
  });
});
