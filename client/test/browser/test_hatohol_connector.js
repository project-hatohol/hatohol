describe('HatoholConnector', function() {
  var TEST_USER = "test-user";
  var TEST_PASSWORD = "test*passwd";

  beforeEach(function(done) {
    HatoholSessionManager.deleteCookie();
    done();
  });

  afterEach(function(done) {
    HatoholDialogObserver.reset();
    done();
  });

  it('get simplest', function(done) {
    HatoholDialogObserver.registerCreatedCallback(function(id, obj) {
      if (id == "hatohol_login_dialog")
        obj.makeInput(TEST_USER, TEST_PASSWORD);
    });

    var params = {
      url: "/test",
      replyCallback: function(data, parser) {
        expect(data).not.to.be(undefined);
        expect(parser).not.to.be(undefined);
        expect(parser.getStatus()).to.be(hatohol.HTERR_OK);
        done();
      },
    };
    var connector = HatoholConnector(params);
  })
});

