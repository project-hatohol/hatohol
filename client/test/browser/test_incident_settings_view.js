describe('IncidentSettingsView', function() {
  var TEST_FIXTURE_ID = 'incidentSettingsViewFixture';
  var incidentSettingsViewHTML;
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
      "type": hatohol.ACTION_INCIDENT_SENDER,
      "workingDirectory": "",
      "command": "1",
      "timeout": 0,
      "ownerUserId": 0,
    },
  ];
  var defaultIncidentTrackers = [
    {
      "id": 1,
      "nickname": "Redmine",
      "type": hatohol.INCIDENT_TRACKER_REDMINE,
      "baseURL": "http://127.0.0.1/redmine/",
      "projectId": "testproject",
      "trackerId": "",
      "userName": "",
    },
  ];

  function getActionsJson(actions) {
    return JSON.stringify({
      apiVersion: hatohol.FACE_REST_API_VERSION,
      errorCode: hatohol.HTERR_OK,
      actions: actions ? actions : defaultActions,
    });
  }

  function getIncidentTrackersJson(incidentTrackers) {
    return JSON.stringify({
      apiVersion: hatohol.FACE_REST_API_VERSION,
      errorCode: hatohol.HTERR_OK,
      incidentTrackers: incidentTrackers ?
        incidentTrackers : defaultIncidentTrackers,
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

  function respond(actionsJson, incidentTrackersJson) {
    var request;

    if (!actionsJson)
      actionsJson = getActionsJson();
    if (!incidentTrackersJson)
      incidentTrackersJson = getIncidentTrackersJson();

    request = this.requests[0];
    request.respond(200, { "Content-Type": "application/json" },
                    actionsJson);

    request = this.requests[1];
    request.respond(200, { "Content-Type": "application/json" },
                    incidentTrackersJson);

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

  beforeEach(function(done) {
    $('body').append($('<div>', { id: TEST_FIXTURE_ID }));
    var setupFixture = function(html) {
      incidentSettingsViewHTML = html;
      $("#" + TEST_FIXTURE_ID).append($("<div>", { id: "main" }))
      $("#main").html(incidentSettingsViewHTML);
      fakeAjax();
      done();
    };

    if (incidentSettingsViewHTML)
      setupFixture(incidentSettingsViewHTML);
    else
      loadFixture("ajax_incident_settings", setupFixture)
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
    var userProfile = new HatoholUserProfile(operator);
    var view = new IncidentSettingsView(userProfile);
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
      "flags": (1 << hatohol.OPPRVLG_GET_ALL_INCIDENT_SETTINGS |
                1 << hatohol.OPPRVLG_DELETE_INCIDENT_SETTING)
    };
    var userProfile = new HatoholUserProfile(operator);
    var view = new IncidentSettingsView(userProfile);
    respond();

    var deleteButton = $('#delete-incident-setting-button');
    var checkboxes = $('.delete-selector .selectcheckbox');
    expect(deleteButton).to.have.length(1);
    expect(checkboxes).to.have.length(1);
    expect(deleteButton.is(":visible")).to.be(true);
    expect(checkboxes.is(":visible")).to.be(true);
  });

  it('with no delete privilege', function() {
    var operator = {
      "userId": 2,
      "name": "guest",
      "flags": 0
    };
    var userProfile = new HatoholUserProfile(operator);
    var view = new IncidentSettingsView(userProfile);
    respond();

    var deleteButton = $('#delete-incident-setting-button');
    var checkboxes = $('.delete-selector .selectcheckbox');
    expect(deleteButton).to.have.length(1);
    expect(checkboxes).to.have.length(1);
    expect(deleteButton.is(":visible")).to.be(false);
    expect(checkboxes.is(":visible")).to.be(false);
  });

  it('with update privilege', function() {
    var operator = {
      "userId": 1,
      "name": "admin",
      "flags": 1 << hatohol.OPPRVLG_UPDATE_INCIDENT_SETTING
    };
    var userProfile = new HatoholUserProfile(operator);
    var view = new IncidentSettingsView(userProfile);
    respond();

    var editColumn = $('.edit-incident-setting-column');
    var editButton = $('#edit-incident-setting1');
    expect(editColumn).to.have.length(1);
    expect(editButton).to.have.length(1);
    expect(editColumn.is(":visible")).to.be(true);
    expect(editButton.is(":visible")).to.be(true);
  });

  it('with no update privilege', function() {
    var operator = {
      "userId": 2,
      "name": "guest",
      "flags": 0
    };
    var userProfile = new HatoholUserProfile(operator);
    var view = new IncidentSettingsView(userProfile);
    respond();

    var editColumn = $('.edit-incident-setting-column');
    var editButton = $('#edit-incident-setting1');
    expect(editColumn).to.have.length(1);
    expect(editButton).to.have.length(1);
    expect(editColumn.is(":visible")).to.be(false);
    expect(editButton.is(":visible")).to.be(false);
  });
});
