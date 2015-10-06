/*
 * Copyright (C) 2013-2015 Project Hatohol
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
  var params = deparam();
  self.options = options || {};
  self.userConfig = null;
  self.columnNames = [];
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

  setupEventsTable();
  setupToggleFilter();
  setupToggleSidebar();

  if (self.options.disableTimeRangeFilter) {
   // Don't enable datetimepicker for tests.
  } else {
    setupTimeRangeFilter();
  }

  if (params && (params.devel == "true")) {
    // I'm not sure why, but sometimes Pizza doesn't work correctly without this
    // hack, especially on Safari. We should investigate & fix it before we
    // enable this feature by default.
    Pizza.init();
    if (self.options.disablePieChart) {
      // Don't enable piechart for tests.
    } else {
      setTimeout(setupPieChart, 100);
    }
  }
  setupApplyFilterButton();

  var status_choices = [gettext('OK'), gettext('Problem'), gettext('Unknown'),
                        gettext('Notification')];
  var severity_choices = [
    gettext('Not classified'), gettext('Information'), gettext('Warning'),
    gettext('Average'), gettext('High'), gettext('Disaster')];

  var columnDefinitions = {
    "monitoringServerName": {
      header: gettext("Monitoring Server"),
      body: renderTableDataMonitoringServer,
    },
    "hostName": {
      header: gettext("Host"),
      body: renderTableDataHostName,
    },
    "eventId": {
      header: gettext("Event ID"),
      body: renderTableDataEventId,
    },
    "status": {
      header: gettext("Status"),
      body: renderTableDataEventStatus,
    },
    "severity": {
      header: gettext("Severity"),
      body: renderTableDataEventSeverity,
    },
    "time": {
      header: gettext("Time"),
      body: renderTableDataEventTime,
    },
    "description": {
      header: gettext("Brief"),
      body: renderTableDataEventDescription,
    },
    "duration": {
      header: gettext("Duration"),
      body: renderTableDataEventDuration,
    },
    "incidentStatus": {
      header: gettext("Treatment"),
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

  // call the constructor of the super class
  HatoholMonitoringView.apply(this, [userProfile]);

  self.pager = new HatoholEventPager();
  start();

  //
  // Private functions
  //
  function start() {
    self.userConfig = new HatoholEventsViewConfig({
      columnDefinitions: columnDefinitions,
	loadedCallback: function(config) {
	  applyConfig(config);

	  updatePager();
	  setupFilterValues();
	  setupCallbacks();

	  load();
	},
	savedCallback: function(config) {
	  applyConfig(config);
	  load();
	},
    });
  }

  function applyConfig(config) {
    self.reloadIntervalSeconds = config.getValue('events.auto-reload.interval');
    self.baseQuery.limit = config.getValue('events.num-rows-per-page');
    self.baseQuery.sortType = config.getValue('events.sort.type');
    self.baseQuery.sortOrder = config.getValue('events.sort.order');
    self.columnNames = config.getValue('events.columns').split(',');
  }

  function updatePager() {
    self.pager.update({
      currentPage: self.currentPage,
      numRecords: self.rawData ? self.rawData["numberOfEvents"] : -1,
      numRecordsPerPage: self.baseQuery.limit,
      selectPageCallback: function(page) {
        if (self.pager.numRecordsPerPage != self.baseQuery.limit)
          self.baseQuery.limit = self.pager.numRecordsPerPage;
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

    if ("minimumSeverity" in query)
      $("#select-severity").val(query.minimumSeverity);
    if ("status" in query)
      $("#select-status").val(query.status);
  }

  function setupTreatmentMenu() {
    var trackers = self.rawData.incidentTrackers;
    var enableIncident = self.rawData["haveIncident"];
    var hasIncidentTypeHatohol = false;
    var hasIncidentTypeOthers = false;

    if (!self.rawData["haveIncident"]) {
      $("#select-incident-container").hide();
      fixupEventsTableHeight();
      return;
    }

    if (typeof trackers == "object") {
      $.map(trackers, function(tracker, key) {
        switch (tracker.type) {
        case hatohol.INCIDENT_TRACKER_HATOHOL:
          hasIncidentTypeHatohol = true;
          break;
        case hatohol.INCIDENT_TRACKER_REDMINE:
        default:
          hasIncidentTypeOthers = true;
          break;
        }
      });
    }

    if (hasIncidentTypeHatohol && !hasIncidentTypeOthers) {
      $("#select-incident-container").show();
      fixupEventsTableHeight();
    } else {
      $("#select-incident-container").hide();
      fixupEventsTableHeight();
    }
  }

  function setupCallbacks() {
    $("#select-severity, #select-status").change(function() {
      if (params && params.oldfilter == "true")
        load();
    });

    self.setupHostQuerySelectorCallback(
      load, '#select-server');

    $('#select-host-group', '#select-host').change(function() {
      if (params && params.oldfilter == "true")
        load();
    });

    $('button.latest-button').click(function() {
      load();
    });

    $("#select-incident").change(function() {
      updateIncidentStatus();
    });

    $('button.btn-apply-all-filter').click(function() {
      load();
    });
  }

  function updateIncidentStatus() {
    var updateIncidentIds = [], unifiedId;
    var incidents = $(".incident.selected");
    var promise, promises = [], errors = [];
    var errorMessage;

    for (var i = 0; i < incidents.length; i++) {
      unifiedId = incidents[i].getAttribute("data-unified-id");
      updateIncidentIds.push(unifiedId);
    }

    for (var idx = 0; idx < updateIncidentIds.length; idx++) {
      promise = applyIncidentStatus(updateIncidentIds[idx], errors);
      promises.push(promise);
    }

    if (promises.length > 0) {
      hatoholInfoMsgBox(gettext("Appling the treatment..."));
      $.when.apply($, promises).done(function() {
        if (errors.length == 0) {
          hatoholInfoMsgBox(gettext("Successfully updated."));
        } else {
          if (errors.length == 1)
            errorMessage = gettext("Failed to update a treatment");
          else
            errorMessage = gettext("Failed to update treatments");
          hatoholErrorMsgBox(errorMessage, { optionMessages: errors });
        }
        load();
      });
    }
  }

  function makeQueryData() {
    var queryData = {};
    queryData.status = $("#select-incident").val();
    return queryData;
  }

  function applyIncidentStatus(updateIncidentId, errors) {
    var deferred = new $.Deferred;
    var url = "/incident";
    url += "/" + updateIncidentId;
    new HatoholConnector({
      url: url,
      request: "PUT",
      data: makeQueryData(),
      replyCallback: function() {
        // nothing to do
      },
      parseErrorCallback: function(reply, parser)  {
        var message = parser.getMessage();
        if (!message) {
          message =
            gettext("An unknown error is occured on changing " +
                    "a treatment of an event with ID: ") +
            updateIncidentId;
        }
        if (parser.optionMessages)
          message += " " + parser.optionMessages;
        errors.push(message);
      },
      completionCallback: function() {
        deferred.resolve();
      },
    });
    return deferred.promise();
  }

  function replyCallback(reply, parser) {
    if (self.incidentTracker)
      hatoholInfoMsgBox(gettext("Successfully updated."));
    else
      hatoholInfoMsgBox(gettext("Successfully created."));

    if (self.succeededCallback)
      self.succeededCallback();
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

  function fixupEventsTableHeight() {
    // TODO: calcurate it more strictly
    var height = $(window).height() - 300 - $(".change-treatment").height();
    $(".event-table-content").height(height);
  }

  function setupEventsTable() {
    fixupEventsTableHeight();
    $(window).resize(function () {
      fixupEventsTableHeight();
    });
  }

  function setupToggleFilter() {
    $("#hideDiv").hide();
    $('#hide').click(function(){
      $("#hideDiv").slideToggle();
      $("#filter-right-glyph").toggle();
      $("#filter-down-glyph").toggle();
    });
  }

  function setupToggleSidebar() {
    if (params && (params.devel != "true")) {
      $("#event-table-area").removeClass("col-md-10");
      $("#event-table-area").addClass("col-md-12");
      $("#SummarySidebar").hide();
      $("#toggle-sidebar").hide();
      return;
    }

    $("#SummarySidebar").show();
    $("#toggle-sidebar").show();
    $("#toggle-sidebar").click(function(){
      $("#SummarySidebar").toggle();
      $("#event-table-area").toggleClass("col-md-12");
      $("#event-table-area").toggleClass("col-md-10");
      $("#sidebar-left-glyph").toggle();
      $("#sidebar-right-glyph").toggle();
    });
  }

  function setupPieChart() {
    Pizza.init(document.body, {always_show_text:true});
  }

  function setupApplyFilterButton() {
    if (params && (params.oldfilter == "true")) {
      $('button.btn-apply-all-filter').hide();
    }
  }

  function setLoading(loading) {
    if (loading) {
      $("#select-severity").attr("disabled", "disabled");
      $("#select-status").attr("disabled", "disabled");
      $("#select-server").attr("disabled", "disabled");
      $("#select-host").attr("disabled", "disabled");
      $("#latest-events-button1").attr("disabled", "disabled");
      $("#latest-events-button2").attr("disabled", "disabled");
    } else {
      $("#select-severity").removeAttr("disabled");
      $("#select-status").removeAttr("disabled");
      $("#select-server").removeAttr("disabled");
      if ($("#select-host option").length > 1)
        $("#select-host").removeAttr("disabled");
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
    if (!self.rawData["haveIncident"])
      return null;
    else
      return event["incident"];
  }

  function getSeverityClass(event) {
    var status = event["type"];
    var severity = event["severity"];
    var severityClass = "severity";

    if (status == hatohol.EVENT_TYPE_BAD)
      return "severity" + escapeHTML(severity);
    else
      return "";
  }

  function renderTableDataMonitoringServer(event, server) {
    var html = "";
    var serverId = event["serverId"];
    var serverURL = getServerLocation(server);
    var nickName = getNickName(server, serverId);

    html += "<td class='" + getSeverityClass(event) + "'>";
    if (serverURL) {
      html += "<a href='" + serverURL + "' target='_blank'>" +
        escapeHTML(nickName) + "</a></td>";
    } else {
      html += escapeHTML(nickName)+ "</td>";
    }

    return html;
  }

  function renderTableDataEventId(event, server) {
    return "<td class='" + getSeverityClass(event) + "'>" +
      escapeHTML(event["eventId"]) + "</td>";
  }

  function renderTableDataEventTime(event, server) {
    var html = "";
    var serverURL = getServerLocation(server);
    var hostId = event["hostId"];
    var triggerId = event["triggerId"];
    var eventId = event["eventId"];
    var clock = event["time"];

    html += "<td class='" + getSeverityClass(event) + "'>";
    if (serverURL && serverURL.indexOf("zabbix") >= 1 &&
	!isSelfMonitoringHost(hostId)) {
      html +=
      "<a href='" + serverURL + "tr_events.php?&triggerid=" +
        triggerId + "&eventid=" + eventId +
        "' target='_blank'>" + escapeHTML(formatDate(clock)) +
        "</a></td>";
    } else {
      html += formatDate(clock) + "</td>";
    }

    return html;
  }

  function renderTableDataHostName(event, server) {
    var html = "";
    var hostId = event["hostId"];
    var serverURL = getServerLocation(server);
    var hostName = getHostName(server, hostId);

    // TODO: Should be built by plugins
    html += "<td class='" + getSeverityClass(event) + "'>";
    if (serverURL && serverURL.indexOf("zabbix") >= 0 &&
        !isSelfMonitoringHost(hostId)) {
      html += "<a href='" + serverURL + "latest.php?&hostid="
              + hostId + "' target='_blank'>" + escapeHTML(hostName)
              + "</a></td>";
    } else if (serverURL && serverURL.indexOf("nagios") >= 0 &&
               !isSelfMonitoringHost(hostId)) {
      html += "<a href='" + serverURL + "cgi-bin/status.cgi?host="
        + hostName + "' target='_blank'>" + escapeHTML(hostName) + "</a></td>";
    } else {
      html += escapeHTML(hostName) + "</td>";
    }

    return html;
  }

  function renderTableDataEventDescription(event, server) {
    var description = getEventDescription(event);

    return "<td class='" + getSeverityClass(event) + "'>" +
      escapeHTML(description) + "</td>";
  }

  function renderTableDataEventStatus(event, server) {
    var status = event["type"];
    var statusClass = "status" + event["status"];

    return "<td class='" + getSeverityClass(event) + " " + statusClass + "'>" +
      status_choices[Number(status)] + "</td>";
  }

  function renderTableDataEventSeverity(event, server) {
    var severity = event["severity"];
    var statusClass = "status" + event["status"];

    return "<td class='" + getSeverityClass(event) + "'>" +
      severity_choices[Number(severity)] + "</td>";
  }

  function renderTableDataEventDuration(event, server) {
    var serverId = event["serverId"];
    var triggerId  = event["triggerId"];
    var clock = event["time"];
    var duration = self.durations[serverId][triggerId][clock];

    return "<td class='" + getSeverityClass(event) + "'>" +
      formatSecond(duration) + "</td>";
  }

  function renderTableDataIncidentStatus(event, server) {
    var html = "", incident = getIncident(event);
    var unifiedId = event["unifiedId"];

    html += "<td class='selectable incident " + getSeverityClass(event) + "'";
    html += "data-unified-id='" + unifiedId + "'";
    html += " style='display:none;'>";

    if (!incident)
      return html + "</td>";

    if (!incident.localtion)
      return html + escapeHTML(incident.status) + "</td>";

    html += "<a href='" + escapeHTML(incident.location)
      + "' target='_blank'>";
    html += escapeHTML(incident.status) + "</a>";
    html += "</td>";

    return html;
  }

  function renderTableDataIncidentPriority(event, server) {
    var html = "", incident = getIncident(event);

    html += "<td class='incident " + getSeverityClass(event) + "'";
    html += " style='display:none;'>";

    if (!incident)
      return html + "</td>";

    html += escapeHTML(incident.priority);
    html += "</td>";

    return html;
  }

  function renderTableDataIncidentAssignee(event, server) {
    var html = "", incident = getIncident(event);

    html += "<td class='incident " + getSeverityClass(event) + "'";
    html += " style='display:none;'>";

    if (!incident)
      return html + "</td>";

    html += escapeHTML(incident.assignee);
    html += "</td>";

    return html;
  }

  function renderTableDataIncidentDoneRatio(event, server) {
    var html = "", incident = getIncident(event);

    html += "<td class='incident " + getSeverityClass(event) + "'";
    html += " style='display:none;'>";

    if (!incident)
      return html + "</td>";

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

      isIncident = (columnName.indexOf("incident") == 0);

      header += '<th';
      header += ' id="column_' + columnName + '"';
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
        html += definition.body(event, server);
      }
      html += "</tr>";
    }

    return html;
  }

  function switchSort() {
    var icon;
    var currentOrder = self.baseQuery.sortOrder;

    if (currentOrder == hatohol.DATA_QUERY_OPTION_SORT_DESCENDING) {
      icon = "up";
      self.baseQuery.sortOrder = hatohol.DATA_QUERY_OPTION_SORT_ASCENDING;
      self.userConfig.saveValue('events.sort.order', self.baseQuery.sortOrder);
      self.startConnection(getQuery(self.currentPage), updateCore);
    } else {
      icon = "down";
      self.baseQuery.sortOrder = hatohol.DATA_QUERY_OPTION_SORT_DESCENDING;
      self.userConfig.saveValue('events.sort.order', self.baseQuery.sortOrder);
      self.startConnection(getQuery(self.currentPage), updateCore);
    }
  }

  function drawTableContents() {
    $("#table thead").empty();
    $("#table thead").append(drawTableHeader());
    $("#table tbody").empty();
    $("#table tbody").append(drawTableBody());

    if (self.rawData["haveIncident"]) {
      $(".incident").show();
    } else {
      $(".incident").hide();
    }

    $('.incident.selectable').on('click', function() {
      $(this).toggleClass('selected');
    });

    setupSortColumn();

    function setupSortColumn() {
      var th = $("#column_time");
      var icon = "down";
      if (self.baseQuery.sortOrder == hatohol.DATA_QUERY_OPTION_SORT_ASCENDING)
        icon = "up";
      th.find("i.sort").remove();
      th.append("<i class='sort glyphicon glyphicon-arrow-" + icon +"'></i>");
      th.click(function() {
        switchSort();
      });
    }
  }

  function updateCore(reply) {
    self.rawData = reply;
    self.userConfig.setServers(reply.servers);
    self.durations = parseData(self.rawData);

    setupFilterValues();
    setupTreatmentMenu();
    drawTableContents();
    updatePager();
    setLoading(false);
    if (self.currentPage == 0)
      self.enableAutoRefresh(load, self.reloadIntervalSeconds);
  }
};

EventsView.prototype = Object.create(HatoholMonitoringView.prototype);
EventsView.prototype.constructor = EventsView;
