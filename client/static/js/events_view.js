/*
 * Copyright (C) 2013-2014 Project Hatohol
 *
 * This file is part of Hatohol.
 *
 * Hatohol is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 2 of the License, or
 * (at your option) any later version.
 *
 * Hatohol is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with Hatohol. If not, see <http://www.gnu.org/licenses/>.
 */

var EventsView = function(userProfile, baseElem) {
  var self = this;
  self.baseElem = baseElem;
  self.currentPage = 0;
  self.limiOfUnifiedId = 0;
  self.rawData = {};
  self.durations = {};

  var status_choices = [gettext('OK'), gettext('Problem'), gettext('Unknown')];
  var severity_choices = [
    gettext('Not classified'), gettext('Information'), gettext('Warning'),
    gettext('Average'), gettext('High'), gettext('Disaster')];

  var connParam =  {
    replyCallback: function(reply, parser) {
      self.updateScreen(reply, updateCore);
    },
    parseErrorCallback: function(reply, parser) {
      hatoholErrorMsgBoxForParser(reply, parser);

      self.setStatus({
        "class" : "danger",
        "label" : gettext("ERROR"),
        "lines" : [ msg ],
      });
    }
  };

  // call the constructor of the super class
  HatoholMonitoringView.apply(userProfile);

  self.userConfig = new HatoholUserConfig(); 
  start();

  //
  // Private functions 
  //
  function start() {
    var DEFAULT_NUM_EVENTS_PER_PAGE = 50;
    var DEFAULT_SORT_TYPE = "time";
    var DEFAULT_SORT_ORDER = hatohol.DATA_QUERY_OPTION_SORT_DESCENDING;
    self.userConfig.get({
      itemNames:['num-events-per-page', 'event-sort-order'],
      successCallback: function(conf) {
        self.numEventsPerPage =
          self.userConfig.findOrDefault(conf, 'num-events-per-page',
                                        DEFAULT_NUM_EVENTS_PER_PAGE);
        self.sortType = 
          self.userConfig.findOrDefault(conf, 'event-sort-type',
                                        DEFAULT_SORT_TYPE);
        self.sortOrder = 
          self.userConfig.findOrDefault(conf, 'event-sort-order',
                                        DEFAULT_SORT_ORDER);
        createPage();
      },
      connectErrorCallback: function(XMLHttpRequest, textStatus, errorThrown) {
        // TODO: implement
      },
    });
  }

  function getEventsURL(loadNextPage) {
    if (loadNextPage) {
      self.currentPage += 1;
      if (!self.limitOfUnifiedId)
        self.limitOfUnifiedId = self.rawData.lastUnifiedEventId;
    } else {
      self.currentPage = 0;
      self.limitOfUnifiedId = 0;
    }

    var query = {
      maximumNumber: self.numEventsPerPage,
      offset:        self.numEventsPerPage * self.currentPage,
      sortType:      self.sortType,
      sortOrder:     self.sortOrder
    };
    return '/events?' + $.param(query);
  };

  function createPage() {
    createUI(self.baseElem);
    setupEvents();
    connParam.url = getEventsURL();
    self.connector = new HatoholConnector(connParam);
  }

  function createUI(elem) {
    var s = '';
    s += '<h2>' + gettext('Events') + '</h2>';

    s += '<form class="form-inline hatohol-filter-toolbar">';
    s += '  <label>' + gettext('Minimum Severity:') + '</label>';
    s += '  <select id="select-severity" class="form-control">';
    s += '    <option>0</option>';
    s += '    <option>1</option>';
    s += '    <option>2</option>';
    s += '    <option>3</option>';
    s += '    <option>4</option>';
    s += '  </select>';
    s += '  <label>' + gettext('Status:') + '</label>';
    s += '  <select id="select-status" class="form-control">';
    s += '    <option value="-1">---------</option>';
    s += '    <option value="0">' + gettext('OK') + '</option>';
    s += '    <option value="1">' + gettext('Problem') + '</option>';
    s += '    <option value="2">' + gettext('Unknown') + '</option>';
    s += '  </select>';
    s += '  <label>' + gettext('Server:') + '</label>';
    s += '  <select id="select-server" class="form-control">';
    s += '    <option>---------</option>';
    s += '  </select>';
    s += '  <label>' + gettext('Host:') + '</label>';
    s += '  <select id="select-host" class="form-control">';
    s += '    <option>---------</option>';
    s += '  </select>';
    s += '  <label for="num-events-per-page">' + gettext("# of events per page") + '</label>';
    s += '  <input type="text" id="num-events-per-page" class="form-control" style="width:4em;">';
    s += '</form>';

    s += '<table class="table table-condensed table-hover" id="table">';
    s += '  <thead>';
    s += '    <tr>';
    s += '      <th data-sort="string">' + gettext('Server') + '</th>';
    s += '      <th data-sort="int">' + gettext('Time') + '</th>';
    s += '      <th data-sort="string">' + gettext('Host') + '</th>';
    s += '      <th data-sort="string">' + gettext('Brief') + '</th>';
    s += '      <th data-sort="int">' + gettext('Status') + '</th>';
    s += '      <th data-sort="int">' + gettext('Severity') + '</th>';
    s += '      <th data-sort="int">' + gettext('Duration') + '</th>';
    /* Not supported yet
    s += '      <th data-sort="int">' + gettext('Comment') + '</th>';
    s += '      <th data-sort="int">' + gettext('Action') + '</th>';
    */
    s += '    </tr>';
    s += '  </thead>';
    s += '  <tbody>';
    s += '  </tbody>';
    s += '</table>';

    s += '<center>';
    s += '<form class="form-inline">';
    s += '  <input id="latest-events-button" type="button" class="btn btn-info" value="' + gettext('Latest events') + '" />';
    s += '  <input id="next-events-button" type="button" class="btn btn-primary" value="' + gettext('To next') + '" />';
    s += '</form>';
    s += '</center>';
    s += '<br>';

    $(elem).append(s);
  }

  function setupEvents() {
    $("#table").stupidtable();
    $("#table").bind('aftertablesort', function(event, data) {
      var th = $(this).find("th");
      th.find("i.sort").remove();
      var icon = data.direction === "asc" ? "up" : "down";
      th.eq(data.column).append("<i class='sort glyphicon glyphicon-arrow-" + icon +"'></i>");
    });

    $("#select-severity").change(function() {
      self.updateScreen(self.rawData, updateCore);
    });
    $("#select-server").change(function() {
      var serverId = $("#select-server").val();
      self.setHostFilterCandidates(self.rawData["servers"], serverId);
      drawTableContents();
    });
    $("#select-host").change(function() {
      drawTableContents();
    });
    $("#select-status").change(function() {
      drawTableContents();
    });

    $('#num-events-per-page').val(self.numEventsPerPage);
    $('#num-events-per-page').change(function() {
      var val = parseInt($('#num-events-per-page').val());
      if (!isFinite(val))
        val = self.numEventsPerPage;
      $('#num-events-per-page').val(val);
      self.numEventsPerPage = val;

      var params = {
        items: {'num-events-per-page': val},
        successCallback: function(){ /* we just ignore it */ },
        connectErrorCallback: function() {
          // TODO: show an error message
        },
      };
      self.userConfig.store(params);
    });

    $('#next-events-button').click(function() {
      var loadNextPage = true;
      connParam.url = getEventsURL(loadNextPage);
      self.connector.start(connParam);
      $(self.baseElem).scrollTop(0);
    });

    $('#latest-events-button').click(function() {
      connParam.url = getEventsURL();
      self.connector.start(connParam);
      $(self.baseElem).scrollTop(0);
    });
  }

  function parseData(replyData) {
    // The structur of durations:
    // {
    //   serverId1: {
    //     triggerId1: {
    //       clock1: duration1,
    //       clock2: duration2,
    //       ...
    //     },
    //     triggerId2: ...
    //   },
    //   serverId2: ...
    // }

    var durations = {};
    var serverId, triggerId;
    var x, event, now, times, durationsForTrigger;

    // extract times from raw data
    for (x = 0; x < replyData["events"].length; ++x) {
      event = replyData["events"][x];
      serverId = event["serverId"];
      triggerId = event["triggerId"];

      if (!durations[serverId])
        durations[serverId] = {};
      if (!durations[serverId][triggerId])
        durations[serverId][triggerId] = [];
      
      durations[serverId][triggerId].push(event["time"]);
    }

    // create durations maps and replace times arrays with them
    for (serverId in durations) {
      for (triggerId in durations[serverId]) {
        times = durations[serverId][triggerId].uniq().sort();
        durationsForTrigger = {};
        for (x = 0; x < times.length; ++x) {
          if (x == times.length - 1) {
            now = parseInt((new Date()).getTime() / 1000);
            durationsForTrigger[times[x]] = now - Number(times[x]);
          } else {
            durationsForTrigger[times[x]] = Number(times[x + 1]) - Number(times[x]);
          }
        }
        durations[serverId][triggerId] = durationsForTrigger;
      }
    }

    return durations;
  }

  function drawTableBody() {
    var serverName, hostName, clock, status, severity, duration;
    var server, event, html = "";
    var x;
    var targetServerId = self.getTargetServerId();
    var targetHostId = self.getTargetHostId();
    var minimumSeverity = $("#select-severity").val();
    var targetStatus = $("#select-status").val();

    for (x = 0; x < self.rawData["events"].length; ++x) {
      event = self.rawData["events"][x];
      if (event["severity"] < minimumSeverity)
        continue;
      if (targetStatus >= 0 && event["type"] != targetStatus)
        continue;

      var serverId = event["serverId"];
      var hostId = event["hostId"];
      server     = self.rawData["servers"][serverId];
      serverName = getServerName(server, serverId);
      hostName   = getHostName(server, hostId);
      clock      = event["time"];
      status     = event["type"];
      severity   = event["severity"];
      duration   = self.durations[serverId][event["triggerId"]][clock];

      if (targetServerId && serverId != targetServerId)
        continue;
      if (targetHostId && hostId != targetHostId)
        continue;

      html += "<tr><td>" + escapeHTML(serverName) + "</td>";
      html += "<td data-sort-value='" + escapeHTML(clock) + "'>" + formatDate(clock) + "</td>";
      html += "<td>" + escapeHTML(hostName) + "</td>";
      html += "<td>" + escapeHTML(event["brief"]) + "</td>";
      html += "<td class='status" + escapeHTML(status) + "' data-sort-value='" + escapeHTML(status) + "'>" + status_choices[Number(status)] + "</td>";
      html += "<td class='severity" + escapeHTML(severity) + "' data-sort-value='" + escapeHTML(severity) + "'>" + severity_choices[Number(severity)] + "</td>";
      html += "<td data-sort-value='" + duration + "'>" + formatSecond(duration) + "</td>";
      /*
      html += "<td>" + "unsupported" + "</td>";
      html += "<td>" + "unsupported" + "</td>";
      */
      html += "</tr>";
    }

    return html;
  }

  function drawTableContents() {
    $("#table tbody").empty();
    $("#table tbody").append(drawTableBody());
  }

  function updateCore(reply) {
    self.rawData = reply;
    self.durations = parseData(self.rawData);

    self.setServerFilterCandidates(self.rawData["servers"]);
    self.setHostFilterCandidates(self.rawData["servers"]);
    drawTableContents();
  }
};

EventsView.prototype = Object.create(HatoholMonitoringView.prototype);
EventsView.prototype.constructor = EventsView;
