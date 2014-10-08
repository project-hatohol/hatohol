describe('UsersView', function() {
  var TEST_FIXTURE_ID = 'usersViewFixture';
  var viewHTML;
  var defaultUsers = [
    {
      "userId": 1,
      "name": "admin",
      "flags": hatohol.ALL_PRIVILEGES
    },
    {
      "userId": 2,
      "name": "guest",
      "flags": 0
    },
    {
      "userId": 2,
      "name": "guest",
      "flags": (1 << hatohol.OPPRVLG_GET_ALL_USER |
                1 << hatohol.OPPRVLG_DELETE_USER)
    }
  ];

  function usersJson(users, servers) {
    return JSON.stringify({
      apiVersion: 3,
      errorCode: hatohol.HTERR_OK,
      users: users ? users : [],
    });
  }

  function fakeAjax() {
    var requests = this.requests = [];
    this.xhr = sinon.useFakeXMLHttpRequest();
    this.xhr.onCreate = function(xhr) {
      requests.push(xhr);
    };
  }

  function restoreAjax() {
    this.xhr.restore();
  }

  function respondUsers(usersJson) {
    var request = this.requests[0];
    request.respond(200, { "Content-Type": "application/json" },
                    usersJson);
  }

  function respond(usersJson, configJson) {
    respondUsers(usersJson);
  }
  
  beforeEach(function(done) {
    var contentId = "main";
    var setupFixture = function() {
      $("#" + TEST_FIXTURE_ID).append($("<div>", { id: contentId }))
      $("#" + contentId).html(viewHTML);
      fakeAjax();
      done();
    };

    $('body').append($('<div>', { id: TEST_FIXTURE_ID }));

    if (viewHTML) {
      setupFixture();
    } else {
      var iframe = $("<iframe>", {
        id: "fixtureFrame",
        src: "../../ajax_users?start=false",
        load: function() {
          viewHTML = $("#" + contentId, this.contentDocument).html();
          setupFixture();
        }
      })
      $("#" + TEST_FIXTURE_ID).append(iframe);
    }
  });

  afterEach(function() {
    restoreAjax();
    //$("#" + TEST_FIXTURE_ID).remove();
  });

  it('with delete privilege', function() {
    var userProfile = new HatoholUserProfile(defaultUsers[2]);
    var view = new UsersView(userProfile);
    respond(usersJson(defaultUsers));
    var heads = $('div#' + TEST_FIXTURE_ID + ' h2');

    expect(heads).to.have.length(1);
    expect($('#table')).to.have.length(1);
    expect($('tr')).to.have.length(defaultUsers.length + 1);
    expect($('#delete-user-button').is(":visible")).to.be(true);
    expect($('.delete-selector .selectcheckbox').is(":visible")).to.be(true);
  });

  it('with no privilege', function() {
    var userProfile = new HatoholUserProfile(defaultUsers[1]);
    var view = new UsersView(userProfile);
    respond(usersJson(defaultUsers));
    var heads = $('div#' + TEST_FIXTURE_ID + ' h2');

    expect(heads).to.have.length(1);
    expect($('#table')).to.have.length(1);
    expect($('tr')).to.have.length(defaultUsers.length + 1);
    expect($('#delete-user-button').is(":visible")).to.be(false);
    expect($('.delete-selector .selectcheckbox').is(":visible")).to.be(false);
  });
});
