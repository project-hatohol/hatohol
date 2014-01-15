describe('HatoholUserProfile', function() {
  var TEST_USER = "test-user-for-user-profile-test";
  var TEST_PASSWORD = "test-user-for-user-profile-test";
  var TEST_FIXTURE_ID = "userProfileTestFixture";
  var CSRF_TOKEN;

  function setLoginDialogCallback() {
    HatoholDialogObserver.registerCreatedCallback(function(id, obj) {
      if (id == "hatohol_login_dialog")
        obj.makeInput(TEST_USER, TEST_PASSWORD);
    });
  }

  function appendFixture() {
    var fixture = $('<div/>', {id: TEST_FIXTURE_ID});
    var html = '';
    html += '<button id="userProfileButton">';
    html += '  <span id="currentUserName"></span>';
    html += '</button>';
    html += '<ul>';
    html += '  <li><a id="logoutMenuItem"></a></li>';
    html += '  <li><a id="changePasswordMenuItem"></a></li>';
    html += '</ul>';
    fixture.html(html);
    $("body").append(fixture);
  }

  function removeFixture() {
    $('#' + TEST_FIXTURE_ID).remove();
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
    appendFixture();
    HatoholSessionManager.deleteCookie();
    setLoginDialogCallback();
    done();
  });

  afterEach(function(done) {
    removeFixture();
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

  it('show user name', function(done) {
    var profile = new HatoholUserProfile();
    profile.addOnLoadCb(function(user) {
      expect($("#currentUserName").text()).to.be(TEST_USER);
      done();
    });
  });
});
