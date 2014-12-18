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

var EventsView = function(userProfile, baseElem) {
  var self = this;
  self.baseElem = baseElem;
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

  var status_choices = [gettext('OK'), gettext('Problem'), gettext('Unknown')];
  var severity_choices = [
    gettext('Not classified'), gettext('Information'), gettext('Warning'),
    gettext('Average'), gettext('High'), gettext('Disaster')];

  // call the constructor of the super class
  HatoholMonitoringView.apply(this, [userProfile]);

  self.userConfig = new HatoholUserConfig(); 
  start();

  //
  // Private functions 
  //
  function start() {
    self.userConfig.get({
      itemNames:['num-events-per-page', 'event-sort-order'],
      successCallback: function(conf) {
        self.baseQuery.limit =
          self.userConfig.findOrDefault(conf, 'num-events-per-page',
                                        self.baseQuery.limit);
        self.baseQuery.sortType =
          self.userConfig.findOrDefault(conf, 'event-sort-type',
                                        self.baseQuery.sortType);
        self.baseQuery.sortOrder =
          self.userConfig.findOrDefault(conf, 'event-sort-order',
                                        self.baseQuery.sortOrder);
        setupFilterValues();
        setupCallbacks();
        load();
      },
      connectErrorCallback: function(XMLHttpRequest, textStatus, errorThrown) {
        showXHRError(XMLHttpRequest);
      },
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
      "minimumSeverity", "status", "triggerId",
    ];
    var i, allParams = deparam(), query = {};
    for (i = 0; i < knownKeys.length; i++) {
      if (knownKeys[i] in allParams)
        query[knownKeys[i]] = allParams[knownKeys[i]];
    }
    return query;
  }

  function getQuery(loadNextPage) {
    if (loadNextPage) {
      self.currentPage += 1;
      if (!self.limitOfUnifiedId)
        self.limitOfUnifiedId = self.rawData.lastUnifiedEventId;
    } else {
      self.currentPage = 0;
      self.limitOfUnifiedId = 0;
    }

    var query = $.extend({}, self.baseQuery, {
      minimumSeverity:  $("#select-severity").val(),
      status:           $("#select-status").val(),
      offset:           self.baseQuery.limit * self.currentPage,
      limitOfUnifiedId: self.limitOfUnifiedId,
    });
    if (self.lastQuery)
      $.extend(query, self.getHostFilterQuery());
    self.lastQuery = query;

    return 'events?' + $.param(query);
  };

  function load(loadNextPage) {
    self.startConnection(getQuery(loadNextPage), updateCore);
    $(document.body).scrollTop(0);
    setLoading(true);
  }

  function setupFilterValues(servers, query) {
    if (!servers && self.rawData && self.rawData.servers)
      servers = self.rawData.servers;

    if (!query)
      query = self.lastQuery ? self.lastQuery : self.baseQuery;

    self.setupHostFilters(servers, query);

    if ('limit' in query)
      $('#num-events-per-page').val(query.limit);
    if ("minimumSeverity" in query)
      $("#select-severity").val(query.minimumSeverity);
    if ("status" in query)
      $("#select-status").val(query.status);
  }

  function setupCallbacks() {
    $("#table").stupidtable();
    $("#table").bind('aftertablesort', function(event, data) {
      var th = $(this).find("th");
      th.find("i.sort").remove();
      var icon = data.direction === "asc" ? "up" : "down";
      th.eq(data.column).append("<i class='sort glyphicon glyphicon-arrow-" + icon +"'></i>");
    });

    $("#select-severity, #select-status").change(function() {
      load();
    });
    self.setupHostQuerySelectorCallback(
      load, '#select-server', '#select-host-group', '#select-host');

    $('#num-events-per-page').change(function() {
      var val = parseInt($('#num-events-per-page').val());
      if (!isFinite(val))
        val = self.baseQuery.limit;
      $('#num-events-per-page').val(val);
      self.baseQuery.limit = val;

      var params = {
        items: {'num-events-per-page': val},
        successCallback: function(){ /* we just ignore it */ },
        connectErrorCallback: function(XMLHttpRequest, textStatus,
                                       errorThrown) {
          showXHRError(XMLHttpRequest);
        },
      };
      self.userConfig.store(params);
    });

    $('#next-events-button').click(function() {
      var loadNextPage = true;
      load(loadNextPage);
    });

    $('#latest-events-button').click(function() {
      load();
    });
  }

  function setLoading(loading) {
    if (loading) {
      $("#select-severity").attr("disabled", "disabled");
      $("#select-status").attr("disabled", "disabled");
      $("#select-server").attr("disabled", "disabled");
      $("#select-host").attr("disabled", "disabled");
      $("#num-events-per-page").attr("disabled", "disabled");
      $("#latest-events-button").attr("disabled", "disabled");
      $("#next-events-button").attr("disabled", "disabled");
    } else {
      $("#select-severity").removeAttr("disabled");
      $("#select-status").removeAttr("disabled");
      $("#select-server").removeAttr("disabled");
      if ($("#select-host option").length > 1)
        $("#select-host").removeAttr("disabled");
      $("#num-events-per-page").removeAttr("disabled");
      $("#latest-events-button").removeAttr("disabled");
      $("#next-events-button").removeAttr("disabled");
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

  function drawTableBody() {
    var serverName, hostName, clock, status, severity, duration;
    var server, event, serverId, hostId, html = "";
    var x;

    for (x = 0; x < self.rawData["events"].length; ++x) {
      event      = self.rawData["events"][x];
      serverId   = event["serverId"];
      hostId     = event["hostId"];
      server     = self.rawData["servers"][serverId];
      serverName = getServerName(server, serverId);
      hostName   = getHostName(server, hostId);
      clock      = event["time"];
      status     = event["type"];
      severity   = event["severity"];
      duration   = self.durations[serverId][event["triggerId"]][clock];
      incident   = event["incident"];

      html += "<tr><td>" + escapeHTML(serverName) + "</td>";
      html += "<td data-sort-value='" + escapeHTML(clock) + "'>" +
        formatDate(clock) + "</td>";
      html += "<td>" + escapeHTML(hostName) + "</td>";
      html += "<td>" + escapeHTML(event["brief"]) + "</td>";
      html += "<td class='status" + escapeHTML(status) +
        "' data-sort-value='" + escapeHTML(status) + "'>" +
        status_choices[Number(status)] + "</td>";
      html += "<td class='severity" + escapeHTML(severity) +
        "' data-sort-value='" + escapeHTML(severity) + "'>" +
        severity_choices[Number(severity)] + "</td>";
      html += "<td data-sort-value='" + duration + "'>" +
        formatSecond(duration) + "</td>";
      if (self.rawData["haveIncident"]) {
        html += "<td class='incident'>";
        html += "<a href='" + escapeHTML(incident.location) + "'>";
        html += escapeHTML(incident.status) + "</a>";
        html += "</td>";
        html += "<td class='incident'>";
        html += escapeHTML(incident.priority);
        html += "</td>";
        html += "<td class='incident'>";
        html += escapeHTML(incident.assignee);
        html += "</td>";
        html += "<td class='incident'>";
        if (incident.status)
          html += escapeHTML(incident.doneRatio) + "%";
        html += "</td>";
      }
      html += "</tr>";
    }

    return html;
  }

  function drawTableContents() {
    if (self.rawData["haveIncident"]) {
      $(".incident").show();
    }
    $("#table tbody").empty();
    $("#table tbody").append(drawTableBody());
  }

  function updateCore(reply) {
    self.rawData = reply;
    self.durations = parseData(self.rawData);

    setupFilterValues();
    drawTableContents();
    setLoading(false);
    self.setAutoReload(load, self.reloadIntervalSeconds);
  }
};

EventsView.prototype = Object.create(HatoholMonitoringView.prototype);
EventsView.prototype.constructor = EventsView;
