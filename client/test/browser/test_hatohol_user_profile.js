describe('HatoholUserProfile', function() {
  var TEST_USER = "test-user-for-user-profile-test";
  var TEST_PASSWORD = "test-user-for-user-profile-test";
  var CSRF_TOKEN;

  function setLoginDialogCallback() {
    HatoholDialogObserver.registerCreatedCallback(function(id, obj) {
      if (id == "hatohol_login_dialog")
        obj.makeInput(TEST_USER, TEST_PASSWORD);
    });
  }

  before(function(done) {
    CSRF_TOKEN = $("*[name=csrfmiddlewaretoken]").val();
    // Add a user for this test case
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
      }
    });
  });

  beforeEach(function(done) {
    HatoholSessionManager.deleteCookie();
    setLoginDialogCallback();
    done();
  });

  afterEach(function(done) {
    HatoholDialogObserver.reset();
    done();
  });

  it('load user', function(done) {
    var profile = new HatoholUserProfile();
    expect(profile.user).to.be(null);
    profile.addOnLoadCb(function(user) {
      var expectedUser = {
        "userId": user.userId, // don't check userId
        "name": TEST_USER,
        "flags": 0
      };
      expect(user).to.eql(expectedUser);
      expect(profile.user).to.eql(expectedUser);
      done();
    });
  });
});
