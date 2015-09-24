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
      "brief":"Test description.",
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
      "brief":"Test description 2.",
      "extendedInfo":"{\"expandedDescription\": \"Expanded test description 2.\"}",
    },
  ];
  var testOptions = {
    disableTimeRangeFilter: true,
  };

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
    var i, haveIncident = false;
    for (i = 0; events && i < events.length; i++) {
      if (events[i].incident)
	haveIncident = true;
    }
    return JSON.stringify({
      apiVersion: 3,
      errorCode: hatohol.HTERR_OK,
      haveIncident: haveIncident,
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

  function getEventTimeString(event) {
    var date = new Date(event.time * 1000);
    var dateString = "";
    dateString += date.getFullYear();
    dateString += "/";
    dateString += padDigit(date.getMonth() + 1, 2);
    dateString += "/";
    dateString += padDigit(date.getDate(), 2);
    dateString += " ";
    dateString += padDigit(date.getHours(), 2);
    dateString += ":";
    dateString += padDigit(date.getMinutes(), 2);
    dateString += ":";
    dateString += padDigit(date.getSeconds(), 2);
    return dateString;
  }

  function testTableContents(serverURL, hostURL, dummyServerInfo, params){
    var view = new EventsView(getOperator(), testOptions);
    var expected = "";

    expected += '<td class="severity1">Problem</td>';
    expected += '<td class="severity1">Information</td>';

    if (params) {
      expected += '<td class="severity1"><a href="' + escapeHTML(params.eventURL) +
                  '" target="_blank">' + escapeHTML(formatDate(1415749496)) +
                  '</a></td>';
    } else {
      expected += '<td class="severity1">' +
                  formatDate(1415749496) + '</td>';
    }

    if (serverURL) {
      expected +=
        '<td class="severity1"><a href="' + escapeHTML(serverURL) + '"' +
        ' target="_blank">Server</a></td>';
    } else {
      expected += '<td class="severity1">Server</td>';
    }

    if (hostURL) {
      expected +=
	'<td class="severity1"><a href="' + escapeHTML(hostURL) + '"' +
        ' target="_blank">Host</a></td>';
    } else {
      expected += '<td class="severity1">Host</td>';
    }

    expected += '<td class="severity1">Test description.</td>';

    respond(eventsJson(dummyEventInfo, dummyServerInfo));
    expect($('#table')).to.have.length(1);
    expect($('tr')).to.have.length(dummyEventInfo.length + 1);
    expect($('tr :eq(1)').html()).to.contain(expected);
    expect($('td :eq(3)').html()).to.match(/[0-9][0-9]:[0-9][0-9]:[0-9][0-9]/);
  }

  function testTableContentsWithExpandedDescription(serverURL, hostURL,
                                                    dummyServerInfo, params)
  {
    var view = new EventsView(getOperator(), testOptions);
    var expected = "";

    expected += '<td class="severity1">Problem</td>';
    expected += '<td class="severity1">Information</td>';
    if (params) {
      expected += '<td class="severity1"><a href="' + escapeHTML(params.eventURL) +
                  '" target="_blank">' + escapeHTML(formatDate(1415759496)) +
                  '</a></td>';
    } else {
      expected += '<td class="severity1">' +
                  formatDate(1415759496) +
                  '</td>';
    }
    expected +=
      '<td class="severity1"><a href="' + escapeHTML(serverURL) + '"' +
      ' target="_blank">Server</a></td>';
    expected += '<td class="severity1"><a href="' + escapeHTML(hostURL) +
                '" target="_blank">Host2</a></td>';
    expected += '<td class="severity1">Expanded test description 2.</td>';

    respond(eventsJson(dummyEventInfo, dummyServerInfo));
    expect($('#table')).to.have.length(1);
    expect($('tr')).to.have.length(dummyEventInfo.length + 1);
    expect($('tr :eq(2)').html()).to.contain(expected);
    expect($('td :eq(3)').html()).to.match(/[0-9][0-9]:[0-9][0-9]:[0-9][0-9]/);
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
    var view = new EventsView(getOperator(), testOptions);
    respond(eventsJson());
    var heads = $('div#' + TEST_FIXTURE_ID + ' h2');
    expect(heads).to.have.length(1);
    //TODO: It requires valid gettext()
    //expect(heads.first().text()).to.be(gettext("event"));
    expect($('#table')).to.have.length(1);
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

  it('new with nagios data without baseURL', function() {
    var nagiosURL = undefined;
    var nagiosStatusURL = undefined;
    var serverInfo = getDummyServerInfo(1);
    serverInfo["1"]["baseURL"] = undefined;
    testTableContents(nagiosURL, nagiosStatusURL, serverInfo);
  });

  it('new with nagios data included baseURL', function() {
    var nagiosURL = "http://192.168.1.100/base/";
    var nagiosStatusURL = undefined;
    testTableContents(nagiosURL, nagiosStatusURL, getDummyServerInfo(1));
  });

  it('With a good event', function() {
    var view = new EventsView(getOperator(), testOptions);
    var serverURL = "http://192.168.1.100/zabbix/";
    var hostURL =
      "http://192.168.1.100/zabbix/latest.php?&hostid=10105";
    var eventURL =
      "http://192.168.1.100/zabbix/tr_events.php?&triggerid=13569&eventid=12332";
    var expected =
      '<td class="">OK</td>' +
      '<td class="">Information</td>' +
      '<td class=""><a href="' + escapeHTML(eventURL) +
      '" target="_blank">' + escapeHTML(formatDate(1415749496)) +
      '</a></td>' +
      '<td class=""><a href="' + escapeHTML(serverURL) + '" target="_blank">Server</a></td>' +
      '<td class=""><a href="' + escapeHTML(hostURL) + '" target="_blank">Host</a></td>' +
      '<td class="">Test description.</td>';
    var events = [
      $.extend({}, dummyEventInfo[0], { type: hatohol.EVENT_TYPE_GOOD })
    ];
    respond(eventsJson(events, getDummyServerInfo(0)));
    expect($('#table')).to.have.length(1);
    expect($('tr')).to.have.length(events.length + 1);
    expect($('tr :eq(1)').html()).to.contain(expected);
  });

  it('Default columns', function() {
    var view = new EventsView(getOperator(), testOptions);
    respond(eventsJson(dummyEventInfo, getDummyServerInfo(0)));
    expect($('tr :eq(0)').text()).to.be(
      "IncidentStatusSeverityTimeMonitoring ServerHostBrief");
    expect($('tr :eq(1)').text()).to.be(
      "ProblemInformation" + getEventTimeString(dummyEventInfo[0]) +
      "ServerHostTest description.");
  });

  it('Customize columns', function() {
    var view = new EventsView(getOperator(), testOptions);
    var configJson =
      '{"events.columns":"duration,severity,status,description,' +
      'hostName,time,monitoringServerName"}';
    respond(eventsJson(dummyEventInfo, getDummyServerInfo(0)),
	    configJson);
    expect($('tr :eq(0)').text()).to.be(
      "DurationSeverityStatusBriefHostTimeMonitoring Server");
    expect($('tr :eq(1)').text()).to.be(
      "02:46:40InformationProblemTest description.Host" +
      getEventTimeString(dummyEventInfo[0]) +
      "Server");
  });

  it('Incident columns', function() {
    var view = new EventsView(getOperator(), testOptions);
    var events = [
      $.extend({}, dummyEventInfo[0], {
	incident: {
	  status: "NONE",
	  priority: "HIGH",
	  assignee: "Tom",
	  doneRatio: 5,
	}
      })
    ];
    var configJson =
      '{"events.columns":"eventId,' +
      'incidentStatus,incidentPriority,incidentAssignee,incidentDoneRatio"}';
    respond(eventsJson(events, getDummyServerInfo(0)),
	    configJson);
    expect($('tr :eq(0)').text()).to.be(
      "Event IDIncidentPriorityAssignee% Done");
    expect($('tr :eq(1)').text()).to.be(
      "12332NONEHIGHTom5%");
  });
});
