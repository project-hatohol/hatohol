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

var TriggersView = function(userProfile) {
  var self = this;
  var rawData;

  self.reloadIntervalSeconds = 60;
  self.baseQuery = {
    limit: 50,
  };
  $.extend(self.baseQuery, getTriggersQueryInURI());
  self.lastQuery = undefined;

  // call the constructor of the super class
  HatoholMonitoringView.apply(this, [userProfile]);

  self.pager = new HatoholPager();
  self.userConfig = new HatoholUserConfig();
  start();

  function start() {
    self.userConfig.get({
      itemNames:['num-triggers-per-page'],
      successCallback: function(conf) {
        self.baseQuery.limit =
          self.userConfig.findOrDefault(conf, 'num-triggers-per-page',
                                        self.baseQuery.limit);
        updatePager();
        setupFilterValues();
        setupCallbacks();
        load();
      },
      connectErrorCallback: function(XMLHttpRequest) {
        showXHRError(XMLHttpRequest);
      },
    });
  }

  function showXHRError(XMLHttpRequest) {
    var errorMsg = "Error: " + XMLHttpRequest.status + ": " +
      XMLHttpRequest.statusText;
    hatoholErrorMsgBox(errorMsg);
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
      numTotalRecords: rawData ? rawData["totalNumberOfTriggers"] : -1,
      numRecordsPerPage: self.baseQuery.limit,
      selectPageCallback: function(page) {
        load(page);
        if (self.pager.numRecordsPerPage != self.baseQuery.limit) {
          self.baseQuery.limit = self.pager.numRecordsPerPage;
          saveConfig({'num-triggers-per-page': self.baseQuery.limit})
        }
      }
    });
  }

  var status_choices = [
    gettext("OK"),
    gettext("Problem"),
    gettext("Unknown")
  ];
  var severity_choices = [
    gettext("Not classified"),
    gettext("Information"),
    gettext("Warning"),
    gettext("Average"),
    gettext("High"),
    gettext("Disaster")
  ];

  function setupFilterValues(servers, query) {
    if (!servers && rawData && rawData.servers)
      servers = rawData.servers;

    if (!query)
      query = self.lastQuery ? self.lastQuery : self.baseQuery;

    self.setupHostFilters(servers, query);

    if ('limit' in query)
      $('#num-triggers-per-page').val(query.limit);
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

    self.setupHostQuerySelectorCallback(
      load, '#select-server', '#select-host-group', '#select-host');
    $("#select-severity, #select-status").change(function() {
      load();
    });
  }

  function setLoading(loading) {
    if (loading) {
      $("#select-severity").attr("disabled", "disabled");
      $("#select-status").attr("disabled", "disabled");
      $("#select-server").attr("disabled", "disabled");
      $("#select-hostgroup").attr("disabled", "disabled");
      $("#select-host").attr("disabled", "disabled");
    } else {
      $("#select-severity").removeAttr("disabled");
      $("#select-status").removeAttr("disabled");
      $("#select-server").removeAttr("disabled");
      if ($("#select-hostgroup option").length > 1)
        $("#select-hostgroup").removeAttr("disabled");
      if ($("#select-host option").length > 1)
        $("#select-host").removeAttr("disabled");
    }
  }

  function drawTableBody(replyData) {
    var serverName, hostName, clock, status, severity;
    var html, server, trigger;
    var x, serverId, hostId;

    html = "";
    for (x = 0; x < replyData["triggers"].length; ++x) {
      trigger    = replyData["triggers"][x];
      serverId   = trigger["serverId"];
      hostId     = trigger["hostId"];
      server     = replyData["servers"][serverId];
      serverName = getServerName(server, serverId);
      hostName   = getHostName(server, hostId);
      clock      = trigger["lastChangeTime"];
      status     = trigger["status"];
      severity   = trigger["severity"];

      html += "<tr><td>" + escapeHTML(serverName) + "</td>";
      html += "<td class='severity" + escapeHTML(severity) +
        "' data-sort-value='" + escapeHTML(severity) + "'>" +
        severity_choices[Number(severity)] + "</td>";
      html += "<td class='status" + escapeHTML(status) +
        "' data-sort-value='" + escapeHTML(status) + "'>" +
        status_choices[Number(status)] + "</td>";
      html += "<td data-sort-value='" + escapeHTML(clock) + "'>" +
        formatDate(clock) + "</td>";
      html += "<td>" + escapeHTML(hostName) + "</td>";
      html += "<td>"
	+ "<a href='/ajax_events?serverId=" + escapeHTML(serverId)
	+ "&triggerId=" + escapeHTML(trigger["id"]) + "'>"
	+ escapeHTML(trigger["brief"])
	+ "</a></td>";
      html += "</tr>";
    }

    return html;
  }

  function drawTableContents(data) {
    $("#table tbody").empty();
    $("#table tbody").append(drawTableBody(data));
  }

  function updateCore(reply) {
    rawData = reply;

    self.setServerFilterCandidates(rawData["servers"]);
    self.setHostgroupFilterCandidates(rawData["servers"]);
    self.setHostFilterCandidates(rawData["servers"]);

    drawTableContents(rawData);
    updatePager();
    setupFilterValues();
    setLoading(false);
    self.setAutoReload(load, self.reloadIntervalSeconds);
  }

  function getTriggersQueryInURI() {
    var knownKeys = [
      "serverId", "hostgroupId", "hostId",
      "limit", "offset",
      "minimumSeverity", "status",
    ];
    var i, allParams = deparam(), query = {};
    for (i = 0; i < knownKeys.length; i++) {
      if (knownKeys[i] in allParams)
        query[knownKeys[i]] = allParams[knownKeys[i]];
    }
    return query;
  }

  function getQuery(page) {
    if (isNaN(page))
      page = 0;
    var query = $.extend({}, self.baseQuery, {
      minimumSeverity: $("#select-severity").val(),
      status:          $("#select-status").val(),
      offset:          self.baseQuery.limit * page
    });
    if (self.lastQuery)
      $.extend(query, self.getHostFilterQuery());
    self.lastQuery = query;
    return 'trigger?' + $.param(query);
  };

  function load(page) {
    self.startConnection(getQuery(page), updateCore);
    setLoading(true);
    self.pager.update({ currentPage: isNaN(page) ? 0 : page });
  }
};

TriggersView.prototype = Object.create(HatoholMonitoringView.prototype);
TriggersView.prototype.constructor = TriggersView;
