var expect = require('expect.js');
var HatoholReplyParser = require("../../static/js/hatohol_reply_parser");

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


  it("not found errorCode", function() {
    var reply = {"apiVersion":2};
    var parser = new HatoholReplyParser(reply);
    var stat = parser.getStatus();
    expect(stat).to.be(HatoholReplyParser.REPLY_STATUS.NOT_FOUND_ERROR_CODE);
  })

});
