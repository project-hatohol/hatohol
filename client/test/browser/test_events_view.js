describe('EventsView', function() {
  var TEST_FIXTURE_ID = 'eventsViewFixture';
  var viewHTML;
  var dummyEventInfo = [
    {
      "unifiedId":44,
      "serverId":1,
      "time":1415749496,
      "type":1,
      "triggerId":"13569",
      "status":1,
      "severity":1,
      "hostId":"10105",
      "brief":"Test discription.",
    },
  ];

  // TODO: we should use actual server response to follow changes of the json
  //       format automatically
  function eventsJson(events, servers) {
    return JSON.stringify({
      apiVersion: 3,
      errorCode: hatohol.HTERR_OK,
      events: events ? events : [],
      servers: servers ? servers : {}
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

  function respondUserConfig(configJson) {
    var request = this.requests[0];
    if (!configJson)
      configJson = '{}';
    request.respond(200, { "Content-Type": "application/json" },
                    configJson);
  }

  function respondEvents(eventsJson) {
    var request = this.requests[1];
    request.respond(200, { "Content-Type": "application/json" },
                    eventsJson);
  }

  function respond(eventsJson, configJson) {
    respondUserConfig(configJson);
    respondEvents(eventsJson);
  }

  function getDummyServerInfo(type){
    var dummyServerInfo = {
      "1": {
        "name": "Server",
        "type": type,
        "ipAddress": "192.168.1.100",
        "hosts": {
          "10105": {
            "name": "Host",
          },
        },
      },
    };
    return dummyServerInfo;
  }

  function testTableContents(serverURL, hostURL, dummyServerInfo){
    var view = new EventsView($('#' + TEST_FIXTURE_ID).get(0));
    var expected =
      '<td><a href="' + escapeHTML(serverURL) + '" target="_blank">Server</a></td>' +
      '<td data-sort-value="1415749496">' +
      formatDate(1415749496) +
      '</td>' +
      '<td><a href="' + escapeHTML(hostURL) + '" target="_blank">Host</a></td>' +
      '<td>Test discription.</td>' +
      '<td class="status1" data-sort-value="1">Problem</td>' +
      '<td class="severity1" data-sort-value="1">Information</td>';

    respond(eventsJson(dummyEventInfo, dummyServerInfo));
    expect($('#table')).to.have.length(1);
    expect($('tr')).to.have.length(dummyEventInfo.length + 1);
    expect($('tr :eq(1)').html()).to.contain(expected);
    expect($('td :eq(6)').html()).to.match(/[0-9][0-9]:[0-9][0-9]:[0-9][0-9]/);
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
        src: "../../ajax_events?start=false",
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
    $("#" + TEST_FIXTURE_ID).remove();
  });

  it('new with empty data', function() {
    var view = new EventsView($('#' + TEST_FIXTURE_ID).get(0));
    respond(eventsJson());
    var heads = $('div#' + TEST_FIXTURE_ID + ' h2');
    expect(heads).to.have.length(1);
    //TODO: It requires valid gettext()
    //expect(heads.first().text()).to.be(gettext("event"));
    expect($('#table')).to.have.length(1);
    expect($('#num-records-per-page').val()).to.be("50");
  });

  it('new with fake zabbix data', function() {
    var zabbixURL = "http://192.168.1.100/zabbix/";
    var zabbixLatestURL =
      "http://192.168.1.100/zabbix/latest.php?&hostid=10105";
    testTableContents(zabbixURL, zabbixLatestURL, getDummyServerInfo(0));
  });

  it('new with fake nagios data', function() {
    var nagiosURL = "http://192.168.1.100/nagios/";
    var nagiosStatusURL =
      "http://192.168.1.100/nagios/cgi-bin/status.cgi?host=Host";
    testTableContents(nagiosURL, nagiosStatusURL, getDummyServerInfo(1));
  });
});
