describe('HatoholConnector', function() {
  var TEST_USER = "test-user";
  var TEST_PASSWORD = "test*pass*d";
  var CSRF_TOKEN;

  function setLoginDialogCallback() {
    HatoholDialogObserver.registerCreatedCallback(function(id, obj) {
      if (id == "hatohol_login_dialog")
        obj.makeInput(TEST_USER, TEST_PASSWORD);
    });
  }

  function checkBasicResponse(data, parser) {
    expect(data).not.to.be(undefined);
    expect(parser).not.to.be(undefined);
    expect(parser.getStatus()).to.be(hatohol.HTERR_OK);
  }

  before(function(done) {
    CSRF_TOKEN = $("*[name=csrfmiddlewaretoken]").val();
    $.ajax({
      url: "/tunnel/test/user",
      type: "POST",
      data: {"user":TEST_USER, "password":TEST_PASSWORD, "flags":0},
      beforeSend: function(xhr) {
        xhr.setRequestHeader("X-CSRFToken", CSRF_TOKEN);
      },
      success: function(data) {
        var parser = new HatoholReplyParser(data);
        expect(parser.getStatus()).to.be(REPLY_STATUS.OK);
        done();
      },
      error: function connectError(XMLHttpRequest, textStatus, errorThrown) {
        expect().fail(textStatus);
        done();
      },
    });
  });

  beforeEach(function(done) {
    HatoholSessionManager.deleteCookie();
    done();
  });

  afterEach(function(done) {
    HatoholDialogObserver.reset();
    done();
  });

  it('get simplest', function(done) {
    setLoginDialogCallback();
    var params = {
      url: "/test",
      replyCallback: function(data, parser) {
        checkBasicResponse(data, parser);
        done();
      },
    };
    var connector = new HatoholConnector(params);
  });

  it('post simple', function(done) {
    setLoginDialogCallback();
    var queryData = {
      csrfmiddlewaretoken: CSRF_TOKEN, "A":123, "S":"foo", "!'^@^`?":"<(.)>"};
    var params = {
      url: "/test",
      request: "POST",
      data: queryData,
      replyCallback: function(data, parser) {
        checkBasicResponse(data, parser);
        expect(data.queryData).to.eql(queryData);
        done();
      },
    };
    var connector = new HatoholConnector(params);
  });

  it('connection error callback', function(done) {
    setLoginDialogCallback();
    var params = {
      url: "/test",
      request: "POST",
      data: {},
      replyCallback: function(data, parser) {
        expect().fail("replyCallback() should not be called.");
        done();
      },
      connectErrorCallback: function(XMLHttpRequest, textStatus, errorThrown) {
        expect(XMLHttpRequest.status).to.be(403);
        done();
      },
    };
    var connector = new HatoholConnector(params);
  });
});

