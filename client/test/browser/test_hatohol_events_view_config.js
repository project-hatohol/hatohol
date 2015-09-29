describe('EventsViewConfig', function() {
  var TEST_FIXTURE_ID = 'eventsViewConfigFixture';
  var viewHTML;
  var testColumnDefinitions = {
    "monitoringServerName": {
      header: "Monitoring Server"
    },
    "hostName": {
      header: "Host"
    },
    "eventId": {
      header: "Event ID"
    },
    "status": {
      header: "Status"
    },
    "severity": {
      header: "Severity"
    },
    "time": {
      header: "Time"
    },
    "description": {
      header: "Brief"
    },
    "duration": {
      header: "Duration"
    },
    "incidentStatus": {
      header: "Incident"
    },
    "incidentPriority": {
      header: "Priority"
    },
    "incidentAssignee": {
      header: "Assignee"
    },
    "incidentDoneRatio": {
      header: "% Done"
    }
  };

  function getOperator() {
    var operator = {
      "userId": 3,
      "name": "foobar",
      "flags": hatohol.ALL_PRIVILEGES
    };
    return new HatoholUserProfile(operator);
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

  function respondUserConfig(configJson) {
    var request = this.requests[0];
    if (!configJson)
      configJson = '{}';
    request.respond(200, { "Content-Type": "application/json" },
                    configJson);
  }

  function respond(eventsJson, configJson) {
    respondUserConfig(configJson);
  }

  function getDummyServerInfo(type){
    var dummyServerInfo = {
      "1": {
        "nickname" : "Server",
        "type": type,
        "ipAddress": "192.168.1.100",
        "baseURL": "http://192.168.1.100/base/",
        "hosts": {
          "10105": {
            "name": "Host",
          },
          "10106": {
            "name": "Host2",
          }
        },
      },
    };
    return dummyServerInfo;
  }

  beforeEach(function(done) {
    var contentId = "main";
    var setupFixture = function() {
      $("#" + TEST_FIXTURE_ID).append($("<div>", { id: contentId }));
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
        src: "../../ajax_events?start=false",
        load: function() {
          viewHTML = $("#" + contentId, this.contentDocument).html();
          setupFixture();
        }
      });
      $("#" + TEST_FIXTURE_ID).append(iframe);
    }
  });

  afterEach(function() {
    restoreAjax();
    $("#" + TEST_FIXTURE_ID).remove();
  });

  it('load on create', function() {
    var loaded = false;
    var config = new HatoholEventsViewConfig({
      columnDefinitions: testColumnDefinitions,
      loadedCallback: function(config) {
	loaded = true;
      },
      savedCallback: function(config) {
      },
    });
    respond();
    expect(loaded).to.be(true);
  });
});
