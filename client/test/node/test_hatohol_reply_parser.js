var expect = require('expect.js');
var HatoholReplyParser = require("../../static/js/hatohol_reply_parser");
var hatohol = require("../../static/js/hatohol_def");

describe("HatoholReplyParser", function() {
  it("null", function() {
    var parser = new HatoholReplyParser(null);
    var stat = parser.getStatus();
    expect(stat).to.be(HatoholReplyParser.REPLY_STATUS.NULL_OR_UNDEFINED);
  })

  it("undefined", function() {
    var parser = new HatoholReplyParser(undefined);
    var stat = parser.getStatus();
    expect(stat).to.be(HatoholReplyParser.REPLY_STATUS.NULL_OR_UNDEFINED);
  })

  it("not found apiVersion", function() {
    var reply = {"errorCode":0};
    var parser = new HatoholReplyParser(reply);
    var stat = parser.getStatus();
    expect(stat).to.be(HatoholReplyParser.REPLY_STATUS.NOT_FOUND_API_VERSION);
  })

  it("not suport API version", function() {
    var reply = {"apiVersion":1};
    var parser = new HatoholReplyParser(reply);
    var stat = parser.getStatus();
    expect(stat).to.be(HatoholReplyParser.REPLY_STATUS.UNSUPPORTED_API_VERSION);
  })

  it("not found errorCode", function() {
    var reply = {"apiVersion":hatohol.FACE_REST_API_VERSION};
    var parser = new HatoholReplyParser(reply);
    var stat = parser.getStatus();
    expect(stat).to.be(HatoholReplyParser.REPLY_STATUS.NOT_FOUND_ERROR_CODE);
  })

  it("errorCode is not OK", function() {
    var reply = {"apiVersion":hatohol.FACE_REST_API_VERSION,
                 errorCode:hatohol.HTERR_UNKOWN_REASON};
    var parser = new HatoholReplyParser(reply);
    var stat = parser.getStatus();
    expect(stat).to.be(HatoholReplyParser.REPLY_STATUS.ERROR_CODE_IS_NOT_OK);
  })
});
