/*
 * Copyright (C) 2015 Project Hatohol
 *
 * This file is part of Hatohol.
 *
 * Hatohol is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License, version 3
 * as published by the Free Software Foundation.
 *
 * Hatohol is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with Hatohol. If not, see
 * <http://www.gnu.org/licenses/>.
 */

describe('TriggersView', function() {
  var TEST_FIXTURE_ID = 'triggersViewFixture';
  var triggersViewHTML;

  var defaultTriggers = [
    {
      "id": "18446744073709550616",
      "status": 0,
      "severity": 5,
      "lastChangeTime": 1422584694,
      "serverId": 1,
      "hostId": "18446744073709551612",
      "brief": "Failed in connecting to Zabbix."
    },
    {
      "id": "14441",
      "status": 0,
      "severity": 1,
      "lastChangeTime": 0,
      "serverId": 1,
      "hostId": "10106",
      "brief": "Host name of zabbix_agentd was changed on {HOST.NAME}",
      "extendedInfo":"{\"expandedDescription\": \"Host name of zabbix_agentd was changed on TestHost0\"}",
    }
  ];
  var defaultServers = {
    "1": {
      "nickname": "Zabbix",
      "type": 0,
      "ipAddress": "192.168.1.100",
      "hosts": {
        "10101": {
          "name": "Host1"
        },
        "10106": {
          "name": "Host2"
        },
        "18446744073709551612": {
          "name": "Zabbix_SELF"
        }
      },
    },
  };

  function getOperator() {
    var operator = {
      "userId": 3,
      "name": "foobar",
      "flags": hatohol.ALL_PRIVILEGES
    };
    return new HatoholUserProfile(operator);
  }

  function triggersJson(triggers, servers) {
    return JSON.stringify({
      apiVersion: hatohol.FACE_REST_API_VERSION,
      errorCode: hatohol.HTERR_OK,
      triggers: triggers ? triggers : [],
      servers: servers ? servers : {}
    });
  }

  function severityRanksJson(severityRanks) {
    if (!severityRanks)
      severityRanks = [];
    return JSON.stringify({
      apiVersion: hatohol.FACE_REST_API_VERSION,
      errorCode: hatohol.HTERR_OK,
      SeverityRanks: severityRanks,
    });
  }

  function fakeAjax() {
    var requests = this.requests = [];
    this.xhr = sinon.useFakeXMLHttpRequest();
    this.xhr.onCreate = function(xhr) {
      requests.push(xhr);
    };
  }

  function respond(config, triggers) {
    var header = { "Content-Type": "application/json" };
    this.requests[0].respond(200, header, config);
    this.requests[1].respond(200, header, severityRanksJson());
    this.requests[2].respond(200, header, triggers);
  }

  function restoreAjax() {
    this.xhr.restore();
  }

  function loadFixture(pathFromTop, onLoad) {
    var iframe = $("<iframe>", {
      id: "loaderFrame",
      src: "../../" + pathFromTop + "?start=false",
      load: function() {
        var html = $("#main", this.contentDocument).html();
        onLoad(html);
        $('#loaderFrame').remove();
      }
    });
    $('body').append(iframe);
  }

  beforeEach(function(done) {
    $('body').append($('<div>', { id: TEST_FIXTURE_ID }));
    var setupFixture = function(html) {
      triggersViewHTML = html;
      $("#" + TEST_FIXTURE_ID).append($("<div>", { id: "main" }));
      $("#main").html(triggersViewHTML);
      fakeAjax();
      done();
    };

    if (triggersViewHTML)
      setupFixture(triggersViewHTML);
    else
      loadFixture("ajax_triggers", setupFixture);
  });

  afterEach(function() {
    restoreAjax();
    $("#" + TEST_FIXTURE_ID).remove();
  });

  // -------------------------------------------------------------------------
  // Test cases
  // -------------------------------------------------------------------------

  it('new with empty data', function() {
    var view = new TriggersView(getOperator());
    var heads = $("div#" + TEST_FIXTURE_ID + " h2");
    respond('{}', triggersJson());

    expect(heads).to.have.length(1);
    expect($("#table")).to.have.length(1);
    expect($("tr")).to.have.length(1);
  });

  it('Base elements', function() {
    var view = new TriggersView(getOperator());
    var heads = $("div#" + TEST_FIXTURE_ID + " h2");
    respond('{}', triggersJson(defaultTriggers));

    expect(heads).to.have.length(1);
    expect($("#table")).to.have.length(1);
    expect($("tr")).to.have.length(defaultTriggers.length + 1);
  });

  it('Without expandedDesctription', function() {
    var view = new TriggersView(getOperator());
    var eventURL = "ajax_events?serverId=1&amp;triggerId=18446744073709550616";
    var expected =
      '<td class="">Zabbix</td>' +
      '<td class="" data-sort-value="5">Disaster</td>' +
      '<td class="status0" data-sort-value="0">OK</td>' +
      '<td class="" data-sort-value="1422584694">' +
      formatDate(1422584694) +
      '</td>' +
      '<td class="">Zabbix_SELF</td>' +
      '<td class=""><a href="' + eventURL + '">Failed in connecting to Zabbix.</a></td>';
    respond('{}', triggersJson(defaultTriggers, defaultServers));
    expect($('#table')).to.have.length(1);
    expect($('tr')).to.have.length(defaultTriggers.length + 1);
    expect($('tr').eq(1).html()).to.be(expected);
  });

  it('With expandedDesctription', function() {
    var view = new TriggersView(getOperator());
    var eventURL = "ajax_events?serverId=1&amp;triggerId=14441";
    var expected =
      '<td class="">Zabbix</td>' +
      '<td class="" data-sort-value="1">Information</td>' +
      '<td class="status0" data-sort-value="0">OK</td>' +
      '<td class="" data-sort-value="0">-</td>' +
      '<td class="">Host2</td>' +
      '<td class=""><a href="' + eventURL + '">Host name of zabbix_agentd was changed on TestHost0</a></td>';
    respond('{}', triggersJson(defaultTriggers, defaultServers));
    expect($('#table')).to.have.length(1);
    expect($('tr')).to.have.length(defaultTriggers.length + 1);
    expect($('tr').eq(2).html()).to.be(expected);
  });

  it('With a problem trigger', function() {
    var view = new TriggersView(getOperator());
    var eventURL = "ajax_events?serverId=1&amp;triggerId=18446744073709550616";
    var expected =
      '<td class="severity5">Zabbix</td>' +
      '<td class="severity5" data-sort-value="5">Disaster</td>' +
      '<td class="status1 severity5" data-sort-value="1">Problem</td>' +
      '<td class="severity5" data-sort-value="1422584694">' +
      formatDate(1422584694) +
      '</td>' +
      '<td class="severity5">Zabbix_SELF</td>' +
      '<td class="severity5"><a href="' + eventURL + '">Failed in connecting to Zabbix.</a></td>';
    var triggers = [$.extend({}, defaultTriggers[0])];
    triggers[0].status = hatohol.TRIGGER_STATUS_PROBLEM;
    respond('{}', triggersJson(triggers, defaultServers));
    expect($('#table')).to.have.length(1);
    expect($('tr')).to.have.length(triggers.length + 1);
    expect($('tr').eq(1).html()).to.be(expected);
  });
});
