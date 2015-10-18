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

  function respondFilters(filtersJson) {
    var request = this.requests[1];
    if (!filtersJson)
      filtersJson = '[]';
    request.respond(200, { "Content-Type": "application/json" },
                    filtersJson);
  }

  function respond(configJson, filtersJson) {
    respondUserConfig(configJson);
    respondFilters(filtersJson);
  }

  function getDummyServerInfo(type){
    var dummyServerInfo = {
      "1": {
        "nickname" : "Server",
        "type": type,
        "ipAddress": "192.168.1.100",
        "baseURL": "http://192.168.1.100/base/",
        "groups": {
          "101": {
            "name": "group1"
          },
          "102": {
            "name": "group2"
          }
        },
        "hosts": {
          "10105": {
            "name": "Host"
          },
          "10106": {
            "name": "Host2"
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

  it('getCurrentFilterConfig with empty data', function() {
    var config = new HatoholEventsViewConfig({
      columnDefinitions: testColumnDefinitions,
    });
    config.setServers(getDummyServerInfo());
    config.reset();
    respond();
    expect(config.getCurrentFilterConfig()).to.eql({
      name: "ALL (31 days)",
      days: 31,
      incident: {
        enable: false,
        selected: []
      },
      status: {
        enable: false,
        selected: []
      },
      severity: {
        enable: false,
        selected: []
      },
      server: {
        enable: false,
        exclude: false,
        selected: []
      },
      hostgroup: {
        enable: false,
        exclude: false,
        selected: []
      },
      host: {
        enable: false,
        exclude: false,
        selected: []
      }
    });
  });

  it('set and get current filter', function() {
    var config = new HatoholEventsViewConfig({
      columnDefinitions: testColumnDefinitions,
    });
    var expected = {
      name: "Sample Filter",
      days: 60,
      incident: {
        enable: true,
        selected: ["NONE", "IN PROGRESS"]
      },
      status: {
        enable: true,
        selected: ["0", "1"]
      },
      severity: {
        enable: true,
        selected: ["2", "3"]
      },
      server: {
        enable: true,
        exclude: true,
        selected: ["1"]
      },
      hostgroup: {
        enable: true,
        exclude: true,
        selected: ["1,101"]
      },
      host: {
        enable: true,
        exclude: true,
        selected: ["1,10106"]
      }
    };
    respond();
    config.setServers(getDummyServerInfo());
    config.reset();
    config.setCurrentFilterConfig(expected);
    expect(config.getCurrentFilterConfig()).to.eql(expected);
  });

  it('convert filter config to filter', function() {
    var config = new HatoholEventsViewConfig({});
    var filterConfig = {
      name: "Sample Filter",
      days: 60,
      incident: {
        enable: true,
        selected: ["NONE", "IN PROGRESS", "DONE"]
      },
      status: {
        enable: true,
        selected: ["0", "1"]
      },
      severity: {
        enable: true,
        selected: ["2", "3"]
      },
      server: {
        enable: true,
        exclude: false,
        selected: ["1"]
      },
      hostgroup: {
        enable: true,
        exclude: true,
        selected: ["1,101"]
      },
      host: {
        enable: true,
        exclude: false,
        selected: ["1,10106"]
      }
    };
    var expected = {
      incidentStatuses: "NONE,IN PROGRESS,DONE",
      statuses: "0,1",
      severities: "2,3",
      selectHosts: [
        { serverId: "1", },
        { serverId: "1", hostId: "10106" },
      ],
      excludeHosts: [
        { serverId: "1", hostgroupId: "101" },
      ],
    };
    var now = Date.now();
    var expectedTime = parseInt(now / 1000 - 60 * 60 * 24 * filterConfig.days, 10);
    var actual = config.createFilter(filterConfig);
    var time = actual.beginTime;
    expected.beginTime = time;

    expect(actual).to.eql(expected);
    expect(time >= expectedTime && time <= expectedTime + 1).to.be(true);
  });
});
