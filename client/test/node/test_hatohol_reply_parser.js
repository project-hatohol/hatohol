var expect = require('expect.js');
var HatoholReplyParser = require("../../static/js/hatohol_reply_parser");

describe("HatoholReplyParser", function() {
  it("null", function() {
    var parser = new HatoholReplyParser(null);
    var stat = parser.getStatus();
    expect(stat).to.be(HatoholReplyParser.REPLY_STATUS.NULL_OR_UNDEFINED);
  })
});
