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
        expect().fail(function() { return textStatus; });
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

  it('specify pathPrefix', function(done) {
    setLoginDialogCallback();
    var params = {
      url: "/hello",
      pathPrefix: "/test",
      data: {},
      replyParser: function(data) {
        return {
          getStatus: function() { return REPLY_STATUS.OK; }
        }
      },
      replyCallback: function(data, parser) {
        expect(data).to.be('Hello');
        done();
      },
      connectErrorCallback: function(XMLHttpRequest, textStatus, errorThrown) {
        expect(XMLHttpRequest.status).to.be(403);
        done();
      },
    };
    var connector = new HatoholConnector(params);
  });

  it('connection error callback', function(done) {
    setLoginDialogCallback();
    // This test creates an error by using the Django's CSRF protection
    // mechanisim that returns HTTP error 403 when a CSRF token is not
    // specified on a 'POST' request.
    var params = {
      url: "/test",
      request: "POST",
      data: {},
      dontSentCsrfToken: true,
      replyCallback: function(data, parser) {
        expect().fail(function() {
          return 'replyCallback() should not be called.';
        });
        done();
      },
      connectErrorCallback: function(XMLHttpRequest, textStatus, errorThrown) {
        expect(XMLHttpRequest.status).to.be(403);
        done();
      },
    };
    var connector = new HatoholConnector(params);
  });

  it('parse error callback', function(done) {
    setLoginDialogCallback();
    var params = {
      url: "/test/error",
      replyCallback: function(data, parser) {
        expect().fail(function() {
          return 'replyCallback() should not be called.';
        });
        done();
      },
      parseErrorCallback: function(data, parser) {
        expect(data).not.to.be(undefined);
        expect(parser).not.to.be(undefined);
        expect(parser.getErrorCode()).to.be(hatohol.HTERR_ERROR_TEST);
        done();
      },
    };
    var connector = new HatoholConnector(params);
  });

  it('specify replyParser', function(done) {
    setLoginDialogCallback();
    var numParserCalled = 0;
    var CustomParser = function() {
      numParserCalled++;
    };
    CustomParser.prototype = Object.create(HatoholReplyParser.prototype);
    CustomParser.prototype.constructor = CustomParser;

    var params = {
      url: "/test",
      replyCallback: function(data, parser) {
        expect(numParserCalled).to.be(1);
        done();
      },
      replyParser: CustomParser,
    };
    var connector = new HatoholConnector(params);
  });
});

