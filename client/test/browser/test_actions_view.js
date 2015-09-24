describe('ActionsView', function() {
  var TEST_FIXTURE_ID = 'actionsViewFixture';
  var actionsViewHTML;
  var defaultActions = [
    {
      "actionId": 1,
      "enableBits": 0,
      "serverId": null,
      "hostId": null,
      "hostgroupId": null,
      "triggerId": null,
      "triggerStatus": null,
      "triggerSeverity": null,
      "triggerSeverityComparatorType": 0,
      "type": 0,
      "workingDirectory": "",
      "command": "hatohol-actor-mail --to-address hoge@example.com",
      "timeout": 100,
      "ownerUserId": 1
    },
  ];

  function getActionsJson(actions) {
    return JSON.stringify({
      apiVersion: hatohol.FACE_REST_API_VERSION,
      errorCode: hatohol.HTERR_OK,
      actions: actions ? actions : defaultActions,
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

  function respond(actionsJson) {
    var request = this.requests[0];
    request.respond(200, { "Content-Type": "application/json" },
                    actionsJson ? actionsJson : getActionsJson());
  }

  function loadFixture(pathFromTop, onLoad) {
    var iframe = $("<iframe>", {
      id: "loaderFrame",
      src: "../../" + pathFromTop + "?start=false",
      load: function() {
        var html = $("#main", this.contentDocument).html();
        onLoad(html);
        $('#loaderFrame').remove()
      }
    })
    $('body').append(iframe);
  }

  function expectDeleteButtonVisibility(operator, expectedVisibility) {
    var userProfile = new HatoholUserProfile(operator);
    var view = new ActionsView(userProfile);
    respond();

    var deleteButton = $('#delete-action-button');
    var checkboxes = $('.delete-selector .selectcheckbox');
    expect(deleteButton).to.have.length(1);
    expect(checkboxes).to.have.length(1);
    expect(deleteButton.is(":visible")).to.be(expectedVisibility);
    expect(checkboxes.is(":visible")).to.be(expectedVisibility);
  }

  function expectEditColumnVisibility(operator, expectedVisibility) {
    var userProfile = new HatoholUserProfile(operator);
    var view = new ActionsView(userProfile);
    respond();

    var editColumn = $('.edit-action-column');
    var editButton = $('#edit-action1');
    expect(editColumn).to.have.length(1);
    expect(editButton).to.have.length(1);
    expect(editColumn.is(":visible")).to.be(expectedVisibility);
    expect(editButton.is(":visible")).to.be(expectedVisibility);
  }

  beforeEach(function(done) {
    $('body').append($('<div>', { id: TEST_FIXTURE_ID }));
    var setupFixture = function(html) {
      actionsViewHTML = html;
      $("#" + TEST_FIXTURE_ID).append($("<div>", { id: "main" }))
      $("#main").html(actionsViewHTML);
      fakeAjax();
      done();
    };

    if (actionsViewHTML)
      setupFixture(actionsViewHTML);
    else
      loadFixture("ajax_actions", setupFixture)
  });

  afterEach(function() {
    restoreAjax();
    $("#" + TEST_FIXTURE_ID).remove();
  });

  it('Base elements', function() {
    var operator = {
      "userId": 1,
      "name": "admin",
      "flags": hatohol.ALL_PRIVILEGES
    };
    var userProfile = new HatoholUserProfile(defaultActions[0]);
    var view = new ActionsView(userProfile);
    respond();
    var heads = $('div#' + TEST_FIXTURE_ID + ' h2');

    expect(heads).to.have.length(1);
    expect($('#table')).to.have.length(1);
    expect($('tr')).to.have.length(defaultActions.length + 1);
  });

  it('with delete privilege', function() {
    var operator = {
      "userId": 1,
      "name": "admin",
      "flags": (1 << hatohol.OPPRVLG_GET_ALL_ACTION |
                1 << hatohol.OPPRVLG_DELETE_ACTION)
    };
    var expected = true;
    expectDeleteButtonVisibility(operator, expected);
  });

  it('with delete_all privilege', function() {
    var operator = {
      "userId": 2,
      "name": "guest",
      "flags": (1 << hatohol.OPPRVLG_GET_ALL_ACTION |
                1 << hatohol.OPPRVLG_DELETE_ALL_ACTION)
    };
    var expected = true;
    expectDeleteButtonVisibility(operator, expected);
  });

  it('with no delete privilege', function() {
    var operator = {
      "userId": 2,
      "name": "guest",
      "flags": 0
    };
    var expected = false;
    expectDeleteButtonVisibility(operator, expected);
  });

  it('with update privilege', function() {
    var operator = {
      "userId": 1,
      "name": "admin",
      "flags": (1 << hatohol.OPPRVLG_UPDATE_ALL_ACTION |
                1 << hatohol.OPPRVLG_UPDATE_ACTION)
    };
    var expected = true;
    expectEditColumnVisibility(operator, expected);
  });

  it('with all update privilege', function() {
    var operator = {
      "userId": 2,
      "name": "guest",
      "flags": (1 << hatohol.OPPRVLG_UPDATE_ALL_ACTION |
                1 << hatohol.OPPRVLG_UPDATE_ACTION)
    };
    var expected = true;
    expectEditColumnVisibility(operator, expected);
  });

  it('with no update privilege', function() {
    var operator = {
      "userId": 2,
      "name": "guest",
      "flags": 0
    };
    var expected = false;
    expectEditColumnVisibility(operator, expected);
  });
});
