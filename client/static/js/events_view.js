/*
 * Copyright (C) 2013-2014 Project Hatohol
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

var EventsView = function(userProfile, options) {
  var self = this;
  self.options = options || {};
  self.reloadIntervalSeconds = 60;
  self.currentPage = 0;
  self.limitOfUnifiedId = 0;
  self.rawData = {};
  self.durations = {};
  self.baseQuery = {
    limit:            50,
    offset:           0,
    limitOfUnifiedId: 0,
    sortType:         "time",
    sortOrder:        hatohol.DATA_QUERY_OPTION_SORT_DESCENDING,
  };
  $.extend(self.baseQuery, getEventsQueryInURI());
  self.lastQuery = undefined;
  self.showToggleAutoRefreshButton();
  self.setupToggleAutoRefreshButtonHandler(load, self.reloadIntervalSeconds);

  if (self.options.disableTimeRangeFilter) {
   // Don't enable datetimepicker for tests.
  } else {
    setupTimeRangeFilter();
  }

  var status_choices = [gettext('OK'), gettext('Problem'), gettext('Unknown'),
                        gettext('Notification')];
  var severity_choices = [
    gettext('Not classified'), gettext('Information'), gettext('Warning'),
    gettext('Average'), gettext('High'), gettext('Disaster')];

  var defaultColumns =
    "monitoringServerName,time,hostName," +
    "description,status,severity,duration," +
    "incidentStatus,incidentPriority,incidentAssignee,incidentDoneRatio";

  // TODO: Replace defaultColumns when the cutomization UI is implemented.
  /*
  var defaultColumns =
    "incidentStatus,status,severity,time," +
    "monitoringServerName,hostName," +
    "description";
  */

  self.columnsConfig = defaultColumns;
  self.columnNames = self.columnsConfig.split(",");

  // call the constructor of the super class
  HatoholMonitoringView.apply(this, [userProfile]);

  self.pager = new HatoholEventPager();
  self.userConfig = new HatoholUserConfig();
  start();

  //
  // Private functions
  //
  var columnDefinitions = {
    "monitoringServerName": {
      header: gettext("Monitoring Server"),
      body: renderTableDataMonitoringServer,
    },
    "eventId": {
      header: gettext("Event ID"),
      body: renderTableDataEventId,
    },
    "time": {
      header: gettext("Time"),
      body: renderTableDataEventTime,
      sortType: "int",
    },
    "hostName": {
      header: gettext("Host"),
      body: renderTableDataHostName,
    },
    "description": {
      header: gettext("Brief"),
      body: renderTableDataEventDescription,
    },
    "status": {
      header: gettext("Status"),
      body: renderTableDataEventStatus,
    },
    "severity": {
      header: gettext("Severity"),
      body: renderTableDataEventSeverity,
    },
    "duration": {
      header: gettext("Duration"),
      body: renderTableDataEventDuration,
    },
    "incidentStatus": {
      header: gettext("Incident"),
      body: renderTableDataIncidentStatus,
    },
    "incidentPriority": {
      header: gettext("Priority"),
      body: renderTableDataIncidentPriority,
    },
    "incidentAssignee": {
      header: gettext("Assignee"),
      body: renderTableDataIncidentAssignee,
    },
    "incidentDoneRatio": {
      header: gettext("% Done"),
      body: renderTableDataIncidentDoneRatio,
    },
  };

  function start() {
    self.userConfig.get({
      itemNames: [
        'num-records-per-page',
        'event-sort-type',
        'event-sort-order'
      ],
      successCallback: function(conf) {
        self.baseQuery.limit =
          self.userConfig.findOrDefault(conf, 'num-records-per-page',
                                        self.baseQuery.limit);
        self.baseQuery.sortType =
          self.userConfig.findOrDefault(conf, 'event-sort-type',
                                        self.baseQuery.sortType);
        self.baseQuery.sortOrder =
          self.userConfig.findOrDefault(conf, 'event-sort-order',
                                        self.baseQuery.sortOrder);
        self.columnsConfig =
          self.userConfig.findOrDefault(conf, 'event-columns',
                                        defaultColumns);
        self.columnNames = self.columnsConfig.split(",");

        updatePager();
        setupFilterValues();
        setupCallbacks();

        var direction =
          (self.baseQuery.sortOrder == hatohol.DATA_QUERY_OPTION_SORT_ASCENDING) ? "asc" : "desc";
        var thTime = $("#table").find("thead th").eq(1);
        thTime.stupidsort(direction);

        load();
      },
      connectErrorCallback: function(XMLHttpRequest, textStatus, errorThrown) {
        showXHRError(XMLHttpRequest);
      },
    });
  }

  function saveConfig(items) {
    self.userConfig.store({
      items: items,
      successCallback: function() {
        // we just ignore it
      },
      connectErrorCallback: function(XMLHttpRequest) {
        showXHRError(XMLHttpRequest);
      },
    });
  }

  function updatePager() {
    self.pager.update({
      currentPage: self.currentPage,
      numRecords: self.rawData ? self.rawData["numberOfEvents"] : -1,
      numRecordsPerPage: self.baseQuery.limit,
      selectPageCallback: function(page) {
        if (self.pager.numRecordsPerPage != self.baseQuery.limit) {
          self.baseQuery.limit = self.pager.numRecordsPerPage;
          saveConfig({'num-records-per-page': self.baseQuery.limit});
        }
        load(page);
      }
    });
  }

  function showXHRError(XMLHttpRequest) {
    var errorMsg = "Error: " + XMLHttpRequest.status + ": " +
      XMLHttpRequest.statusText;
    hatoholErrorMsgBox(errorMsg);
  }

  function getEventsQueryInURI() {
    var knownKeys = [
      "serverId", "hostgroupId", "hostId",
      "limit", "offset", "limitOfUnifiedId",
      "sortType", "sortOrder",
      "type", "minimumSeverity", "status", "triggerId",
    ];
    var i, allParams = deparam(), query = {};
    for (i = 0; i < knownKeys.length; i++) {
      if (knownKeys[i] in allParams)
        query[knownKeys[i]] = allParams[knownKeys[i]];
    }
    return query;
  }

  function getQuery(page) {
    if (!page) {
      self.limitOfUnifiedId = 0;
    } else {
      if (!self.limitOfUnifiedId)
        self.limitOfUnifiedId = self.rawData.lastUnifiedEventId;
    }

    var query = $.extend({}, self.baseQuery, {
      minimumSeverity:  $("#select-severity").val(),
      type:             $("#select-status").val(),
      offset:           self.baseQuery.limit * self.currentPage,
      limitOfUnifiedId: self.limitOfUnifiedId,
    });

    var beginTime, endTime;
    if ($('#begin-time').val()) {
      beginTime = new Date($('#begin-time').val());
      query.beginTime = parseInt(beginTime.getTime() / 1000);
    }
    if ($('#end-time').val()) {
      endTime = new Date($('#end-time').val());
      query.endTime = parseInt(endTime.getTime() / 1000);
    }

    if (self.lastQuery)
      $.extend(query, self.getHostFilterQuery());
    self.lastQuery = query;

    return 'events?' + $.param(query);
  };

  function load(page) {
    self.displayUpdateTime();
    setLoading(true);
    if (!isNaN(page)) {
      self.currentPage = page;
      self.disableAutoRefresh();
    } else {
      self.currentPage = 0;
    }
    self.startConnection(getQuery(self.currentPage), updateCore);
    $(document.body).scrollTop(0);
  }

  function setupFilterValues(servers, query) {
    if (!servers && self.rawData && self.rawData.servers)
      servers = self.rawData.servers;

    if (!query)
      query = self.lastQuery ? self.lastQuery : self.baseQuery;

    self.setupHostFilters(servers, query);

    if ('limit' in query)
      $('#num-records-per-page').val(query.limit);
    if ("minimumSeverity" in query)
      $("#select-severity").val(query.minimumSeverity);
    if ("status" in query)
      $("#select-status").val(query.status);
  }

  function setupCallbacks() {
    $("#table").stupidtable();
    $("#table").bind('aftertablesort', function(event, data) {
      var icon;
      if (data.column == 1) { // "Time" column
        if (data.direction === "asc") {
          icon = "up";
          if (self.baseQuery.sortOrder != hatohol.DATA_QUERY_OPTION_SORT_ASCENDING) {
            self.baseQuery.sortOrder = hatohol.DATA_QUERY_OPTION_SORT_ASCENDING;
            saveConfig({'event-sort-order': self.baseQuery.sortOrder});
            self.startConnection(getQuery(self.currentPage), updateCore);
          }
        } else {
          icon = "down";
          if (self.baseQuery.sortOrder != hatohol.DATA_QUERY_OPTION_SORT_DESCENDING) {
            self.baseQuery.sortOrder = hatohol.DATA_QUERY_OPTION_SORT_DESCENDING;
            saveConfig({'event-sort-order': self.baseQuery.sortOrder});
            self.startConnection(getQuery(self.currentPage), updateCore);
          }
        }
      }
      var th = $(this).find("th");
      th.find("i.sort").remove();
      if (icon)
        th.eq(data.column).append("<i class='sort glyphicon glyphicon-arrow-" + icon +"'></i>");
    });

    $("#select-severity, #select-status").change(function() {
      load();
    });
    self.setupHostQuerySelectorCallback(
      load, '#select-server', '#select-host-group', '#select-host');

    $('#num-records-per-page').change(function() {
      var val = parseInt($('#num-records-per-page').val());
      if (!isFinite(val))
        val = self.baseQuery.limit;
      $('#num-records-per-page').val(val);
      self.baseQuery.limit = val;

      var params = {
        items: {'num-records-per-page': val},
        successCallback: function(){ /* we just ignore it */ },
        connectErrorCallback: function(XMLHttpRequest, textStatus,
                                       errorThrown) {
          showXHRError(XMLHttpRequest);
        },
      };
      self.userConfig.store(params);
    });

    $('button.latest-button').click(function() {
      load();
    });
  }

  function formatDateTimeWithZeroSecond(d) {
    var t = "" + d.getFullYear() + "/";
    t += padDigit((d.getMonth() + 1), 2);
    t += "/";
    t += padDigit(d.getDate(), 2);
    t += " ";
    t += padDigit(d.getHours(), 2);
    t += ":";
    t += padDigit(d.getMinutes(), 2);
    t += ":00";
    return t;
  }

  function setupTimeRangeFilter() {
    $('#begin-time').datetimepicker({
      format: 'Y/m/d H:i:s',
      onSelectDate: function(currentTime, $input) {
        $('#begin-time').val(formatDateTimeWithZeroSecond(currentTime));
      },
      onSelectTime: function(currentTime, $input) {
        $('#begin-time').val(formatDateTimeWithZeroSecond(currentTime));
      },
      onChangeDateTime: function(currentTime, $input) {
        load();
      }
    });
    $('#end-time').datetimepicker({
      format: 'Y/m/d H:i:s',
      onDateTime: function(currentTime, $input) {
        $('#end-time').val(formatDateTimeWithZeroSecond(currentTime));
      },
      onSelectTime: function(currentTime, $input) {
        $('#end-time').val(formatDateTimeWithZeroSecond(currentTime));
      },
      onChangeDateTime: function(currentTime, $input) {
        load();
      }
    });
  }

  function setLoading(loading) {
    if (loading) {
      $("#select-severity").attr("disabled", "disabled");
      $("#select-status").attr("disabled", "disabled");
      $("#select-server").attr("disabled", "disabled");
      $("#select-host").attr("disabled", "disabled");
      $("#num-records-per-page").attr("disabled", "disabled");
      $("#latest-events-button1").attr("disabled", "disabled");
      $("#latest-events-button2").attr("disabled", "disabled");
    } else {
      $("#select-severity").removeAttr("disabled");
      $("#select-status").removeAttr("disabled");
      $("#select-server").removeAttr("disabled");
      if ($("#select-host option").length > 1)
        $("#select-host").removeAttr("disabled");
      $("#num-records-per-page").removeAttr("disabled");
      $("#latest-events-button1").removeAttr("disabled");
      $("#latest-events-button2").removeAttr("disabled");
    }
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

  function isSelfMonitoringHost(hostId) {
    return  hostId == "__SELF_MONITOR";
  }

  function generateTimeColumn(serverURL, hostId, triggerId, eventId, clock) {
    var html = "";
    if (serverURL.indexOf("zabbix") >= 1 && !isSelfMonitoringHost(hostId)) {
      html += "<td><a href='" + serverURL + "tr_events.php?&triggerid="
              + triggerId + "&eventid=" + eventId
              + "' target='_blank'>" + escapeHTML(formatDate(clock))
              + "</a></td>";
    } else {
      html += "<td data-sort-value='" + escapeHTML(clock) + "'>" +
              formatDate(clock) + "</td>";
    }
    return html;
  }

  function generateHostColumn(serverURL, hostId, hostName) {
    var html = "";
    if (serverURL.indexOf("zabbix") >= 0 && !isSelfMonitoringHost(hostId)) {
      html += "<td><a href='" + serverURL + "latest.php?&hostid="
              + hostId + "' target='_blank'>" + escapeHTML(hostName)
              + "</a></td>";
    } else if (serverURL.indexOf("nagios") >= 0 && !isSelfMonitoringHost(hostId)) {
      html += "<td><a href='" + serverURL + "cgi-bin/status.cgi?host="
        + hostName + "' target='_blank'>" + escapeHTML(hostName) + "</a></td>";
    } else {
      html += "<td>" + escapeHTML(hostName) + "</td>";
    }
    return html;
  }

  function getEventDescription(event) {
    var extendedInfo, name;

    try {
      extendedInfo = JSON.parse(event["extendedInfo"]);
      name = extendedInfo["expandedDescription"];
    } catch(e) {
    }
    return name ? name : event["brief"];
  }

  function getIncident(event) {
    if (self.rawData["haveIncident"])
      return null;
    else
      return event["incident"];
  }

  function renderTableDataMonitoringServer(event, server) {
    var html;
    var serverId = event["serverId"];
    var serverURL = getServerLocation(server);
    var nickName = getNickName(server, serverId);

    if (serverURL) {
      html = "<td><a href='" + serverURL + "' target='_blank'>" +
        escapeHTML(nickName) + "</a></td>";
    } else {
      html = "<td>" + escapeHTML(nickName)+ "</td>";
    }

    return html;
  }

  function renderTableDataEventId(event, server) {
      return "<td>" + escapeHTML(event["eventId"]) + "</td>";
  }

  function renderTableDataEventTime(event, server) {
      var html;
      var serverURL = getServerLocation(server);
      var hostId = event["hostId"];
      var triggerId = event["triggerId"];
      var eventId = event["eventId"];
      var clock = event["time"];

      if (serverURL) {
        html = generateTimeColumn(serverURL, hostId, triggerId, eventId, clock);
      } else {
        html = "<td data-sort-value='" + escapeHTML(clock) + "'>" +
          formatDate(clock) + "</td>";
      }

      return html;
  }

  function renderTableDataHostName(event, server) {
    var html;
    var hostId = event["hostId"];
    var serverURL = getServerLocation(server);
    var hostName = getHostName(server, hostId);

    if (serverURL) {
      html = generateHostColumn(serverURL, hostId, hostName);
    } else {
      html = "<td>" + escapeHTML(hostName) + "</td>";
    }

    return html;
  }

  function renderTableDataEventDescription(event, server) {
    var description = getEventDescription(event);

    return "<td>" + escapeHTML(description) + "</td>";
  }

  function renderTableDataEventStatus(event, server) {
    var status = event["type"];

    return "<td class='status" + escapeHTML(status) +
      "' data-sort-value='" + escapeHTML(status) + "'>" +
      status_choices[Number(status)] + "</td>";
  }

  function renderTableDataEventSeverity(event, server) {
    var status = event["type"];
    var severity = event["severity"];
    var severityClass = "severity";

    if (status == hatohol.EVENT_TYPE_BAD)
      severityClass += escapeHTML(severity);

    return "<td class='" + severityClass +
      "' data-sort-value='" + escapeHTML(severity) + "'>" +
      severity_choices[Number(severity)] + "</td>";
  }

  function renderTableDataEventDuration(event, server) {
    var serverId = event["serverId"];
    var triggerId  = event["triggerId"];
    var clock = event["time"];
    var duration = self.durations[serverId][triggerId][clock];

    return "<td data-sort-value='" + duration + "'>" +
      formatSecond(duration) + "</td>";
  }

  function renderTableDataIncidentStatus(event, server) {
    var html = "", incident = getIncident(event);

    if (!incident)
      return "<td></td>";

    html += "<td class='incident'>";
    html += "<a href='" + escapeHTML(incident.location)
      + "' target='_blank'>";
    html += escapeHTML(incident.status) + "</a>";
    html += "</td>";

    return html;
  }

  function renderTableDataIncidentPriority(event, server) {
    var html = "", incident = getIncident(event);

    if (!incident)
      return "<td></td>";

    html += "<td class='incident'>";
    html += escapeHTML(incident.priority);
    html += "</td>";

    return html;
  }

  function renderTableDataIncidentAssignee(event, server) {
    var html = "", incident = getIncident(event);

    if (!incident)
      return "<td></td>";

    html += "<td class='incident'>";
    html += escapeHTML(incident.assignee);
    html += "</td>";

    return html;
  }

  function renderTableDataIncidentDoneRatio(event, server) {
    var html = "", incident = getIncident(event);

    if (!incident)
      return "<td></td>";

    html += "<td class='incident'>";
    if (incident.status)
      html += escapeHTML(incident.doneRatio) + "%";
    html += "</td>";

    return html;
  }

  function drawTableHeader() {
    var i, definition, columnName, isIncident = false;
    var header = '<tr>';

    for (i = 0; i < self.columnNames.length; i++) {
      columnName = self.columnNames[i];
      definition = columnDefinitions[columnName];
      if (!definition) {
        console.error("Unknown column: " + columnName);
        continue;
      }

      if (columnName.indexOf("incident") == 0)
        isIncident = true;

      header += '<th';
      if (definition.sortType)
        header += ' data-sort="' + definition.sortType + '"';
      if (isIncident)
        header += ' class="incident" style="display:none;"';
      header += '>';
      header += definition.header;
      header += '</th>';
    }

    header += '</tr>';

    return header;
  }

  function drawTableBody() {
    var html = "";
    var x, y, serverId, server, event, columnName, definition;
    var haveIncident = self.rawData["haveIncident"];

    for (x = 0; x < self.rawData["events"].length; ++x) {
      event = self.rawData["events"][x];
      serverId = event["serverId"];
      server = self.rawData["servers"][serverId];

      html += "<tr>"
      for (y = 0; y < self.columnNames.length; y++) {
        columnName = self.columnNames[y];
        definition = columnDefinitions[columnName];
        if (!definition)
          continue;
        if (columnName.indexOf("incident") == 0 && !haveIncident)
          continue;
        html += definition.body(event, server);
      }
      html += "</tr>";
    }

    return html;
  }

  function drawTableContents() {
    $("#table thead").empty();
    $("#table thead").append(drawTableHeader());
    $("#table tbody").empty();
    $("#table tbody").append(drawTableBody());
    if (self.rawData["haveIncident"]) {
      $(".incident").show();
    }
  }

  function updateCore(reply) {
    self.rawData = reply;
    self.durations = parseData(self.rawData);

    setupFilterValues();
    drawTableContents();
    updatePager();
    setLoading(false);
    if (self.currentPage == 0)
      self.enableAutoRefresh(load, self.reloadIntervalSeconds);
  }
};

EventsView.prototype = Object.create(HatoholMonitoringView.prototype);
EventsView.prototype.constructor = EventsView;
