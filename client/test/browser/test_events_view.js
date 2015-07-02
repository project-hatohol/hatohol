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
      "eventId": "12332",
      "status":1,
      "severity":1,
      "hostId":"10105",
      "brief":"Test discription.",
    },
    {
      "unifiedId":43,
      "serverId":1,
      "time":1415759496,
      "type":1,
      "triggerId":"13569",
      "eventId": "18483",
      "status":1,
      "severity":1,
      "hostId":"10106",
      "brief":"Test discription 2.",
      "extendedInfo":"{\"expandedDescription\": \"Expanded test description 2.\"}",
    },
  ];

  function getOperator() {
    var operator = {
      "userId": 3,
      "name": "foobar",
      "flags": hatohol.ALL_PRIVILEGES
    };
    return new HatoholUserProfile(operator);
  }

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

  function testTableContents(serverURL, hostURL, dummyServerInfo, params){
    var view = new EventsView(getOperator());
    var expected;

    if (serverURL) {
      expected =
        '<td><a href="' + escapeHTML(serverURL) + '" target="_blank">Server</a></td>';
    } else {
      expected = '<td>Server</td>';
    }

    if (params) {
      expected += '<td><a href="' + escapeHTML(params.eventURL) +
                  '" target="_blank">' + escapeHTML(formatDate(1415749496)) +
                  '</a></td>';
    } else {
      expected += '<td data-sort-value="1415749496">' +
                  formatDate(1415749496) + '</td>';
    }

    if (hostURL) {
      expected =
	'<td><a href="' + escapeHTML(hostURL) + '" target="_blank">Host</a></td>';
    } else {
      expected = '<td>Host</td>';
    }

    expected +=
      '<td>Test discription.</td>' +
      '<td class="status1" data-sort-value="1">Problem</td>' +
      '<td class="severity1" data-sort-value="1">Information</td>';

    respond(eventsJson(dummyEventInfo, dummyServerInfo));
    expect($('#table')).to.have.length(1);
    expect($('tr')).to.have.length(dummyEventInfo.length + 1);
    expect($('tr :eq(1)').html()).to.contain(expected);
    expect($('td :eq(6)').html()).to.match(/[0-9][0-9]:[0-9][0-9]:[0-9][0-9]/);
  }

  function testTableContentsWithExpandedDescription(serverURL, hostURL,
                                                    dummyServerInfo, params)
  {
    var view = new EventsView(getOperator());
    var expected =
      '<td><a href="' + escapeHTML(serverURL) + '" target="_blank">Server</a></td>';
    if (params) {
      expected += '<td><a href="' + escapeHTML(params.eventURL) +
                  '" target="_blank">' + escapeHTML(formatDate(1415759496)) +
                  '</a></td>';
    } else {
      expected += '<td data-sort-value="1415759496">' +
                  formatDate(1415759496) +
                  '</td>';
    }
    expected += '<td><a href="' + escapeHTML(hostURL) +
                '" target="_blank">Host2</a></td>' +
                '<td>Expanded test description 2.</td>' +
                '<td class="status1" data-sort-value="1">Problem</td>' +
                '<td class="severity1" data-sort-value="1">Information</td>';

    respond(eventsJson(dummyEventInfo, dummyServerInfo));
    expect($('#table')).to.have.length(1);
    expect($('tr')).to.have.length(dummyEventInfo.length + 1);
    expect($('tr :eq(2)').html()).to.contain(expected);
    expect($('td :eq(6)').html()).to.match(/[0-9][0-9]:[0-9][0-9]:[0-9][0-9]/);
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

  afterEach(function(done) {
    setTimeout(function() {
      // Wait completing stupidsort() in EventsView
      // TODO: Don't use stupidtable then remove this setTimeout()
      // (EventsView doesn't use its sort function anymore).
      restoreAjax();
      $("#" + TEST_FIXTURE_ID).remove();
      done();
    }, 20);
  });

  it('new with empty data', function() {
    var view = new EventsView(getOperator());
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
    var params =
      {eventURL: "http://192.168.1.100/zabbix/tr_events.php?&triggerid=13569&eventid=12332"};
    testTableContents(zabbixURL, zabbixLatestURL, getDummyServerInfo(0), params);
  });

  it('new with fake zabbix data included expanded description', function() {
    var zabbixURL = "http://192.168.1.100/zabbix/";
    var zabbixLatestURL =
      "http://192.168.1.100/zabbix/latest.php?&hostid=10106";
    var params =
      {eventURL: "http://192.168.1.100/zabbix/tr_events.php?&triggerid=13569&eventid=18483"};
    testTableContentsWithExpandedDescription(zabbixURL, zabbixLatestURL,
                                             getDummyServerInfo(0), params);
  });

  it('new with fake nagios data', function() {
    // issue #839
    //var nagiosURL = "http://192.168.1.100/nagios/";
    //var nagiosStatusURL =
    //  "http://192.168.1.100/nagios/cgi-bin/status.cgi?host=Host";
    var nagiosURL = undefined;
    var nagiosStatusURL = undefined;
    testTableContents(nagiosURL, nagiosStatusURL, getDummyServerInfo(1));
  });

  it('new with nagios data included baseURL', function() {
    var nagiosURL = "http://192.168.1.100/base/";
    var nagiosStatusURL = undefined;
    testTableContents(nagiosURL, nagiosStatusURL, getDummyServerInfo(1));
  });

  it('With a good event', function() {
    var view = new EventsView(getOperator());
    var serverURL = "http://192.168.1.100/zabbix/";
    var hostURL =
      "http://192.168.1.100/zabbix/latest.php?&hostid=10105";
    var eventURL =
      "http://192.168.1.100/zabbix/tr_events.php?&triggerid=13569&eventid=12332";
    var expected =
      '<td><a href="' + escapeHTML(serverURL) + '" target="_blank">Server</a></td>' +
      '<td><a href="' + escapeHTML(eventURL) +
      '" target="_blank">' + escapeHTML(formatDate(1415749496)) +
      '</a></td>' +
      '<td><a href="' + escapeHTML(hostURL) + '" target="_blank">Host</a></td>' +
      '<td>Test discription.</td>' +
      '<td class="status0" data-sort-value="0">OK</td>' +
      '<td class="severity" data-sort-value="1">Information</td>';
    var events = [
      $.extend({}, dummyEventInfo[0], { type: hatohol.EVENT_TYPE_GOOD })
    ];
    respond(eventsJson(events, getDummyServerInfo(0)));
    expect($('#table')).to.have.length(1);
    expect($('tr')).to.have.length(events.length + 1);
    expect($('tr :eq(1)').html()).to.contain(expected);
  });
});
