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

var OverviewTriggers = function(userProfile) {
  var self = this;
  var rawData, parsedData;

  self.reloadIntervalSeconds = 60;
  self.baseQuery = {
    limit:  0,
    offset: 0,
  };
  $.extend(self.baseQuery, getTriggersQueryInURI());
  self.lastQuery = undefined;
  self.severityRanksMap = {};
  self.rawSeverityRankData = {};
  self.showToggleAutoRefreshButton();
  self.setupToggleAutoRefreshButtonHandler(load, self.reloadIntervalSeconds);

  var triggerPropertyChoices = {
    severity: [
      { value: "0", label: gettext("Not classified") },
      { value: "1", label: gettext("Information") },
      { value: "2", label: gettext("Warning") },
      { value: "3", label: gettext("Average") },
      { value: "4", label: gettext("High") },
      { value: "5", label: gettext("Disaster") },
    ],
  };

  // call the constructor of the super class
  HatoholMonitoringView.apply(this, [userProfile]);

  start();

  function start() {
    $.when(loadSeverityRank()).done(function() {
      self.startConnection(getQuery(true), updateFilter);
      $("#select-severity").attr("disabled", "disabled");
      $("#select-status").attr("disabled", "disabled");
    }).fail(function() {
      hatoholInfoMsgBox(gettext("Failed to get the configuration!"));
      self.startConnection(getQuery(true), updateFilter);
    });
  }

  self.setupHostQuerySelectorCallback(
    load, '#select-server', '#select-host-group', '#select-host');

  $("#select-severity, #select-status").change(function() {
    load();
  });

  $('button.latest-button').click(function() {
    load();
  });

  function parseData(replyData, minimum) {
    var parsedData = {};
    var nickName, hostName, triggerName;
    var nickNames, triggerNames, hostNames;
    var server, trigger;
    var x;
    var targetSeverity = $("#select-severity").val();
    var targetStatus = $("#select-status").val();

    parsedData.hosts  = {};
    parsedData.values = {};

    nickNames = [];
    triggerNames = [];
    hostNames = {};

    for (x = 0; x < replyData["triggers"].length; ++x) {
      trigger = replyData["triggers"][x];
      if (trigger["severity"] < targetSeverity)
        continue;
      if (targetStatus >= 0 && trigger["status"] != targetStatus)
        continue;

      var serverId = trigger["serverId"];
      var hostId   = trigger["hostId"];
      server      = replyData["servers"][serverId];
      nickName    = getNickName(server, serverId);
      hostName    = getHostName(server, hostId);
      triggerName = trigger["brief"];

      nickNames.push(nickName);
      triggerNames.push(triggerName);
      if (!hostNames[nickName])
        hostNames[nickName] = [];
      hostNames[nickName].push(hostName);

      if (!parsedData.values[nickName])
        parsedData.values[nickName] = {};
      if (!parsedData.values[nickName][hostName])
        parsedData.values[nickName][hostName] = {};

      if (!parsedData.values[nickName][hostName][triggerName])
        parsedData.values[nickName][hostName][triggerName] = trigger;
   }

    parsedData.nicknames  = nickNames.uniq().sort();
    parsedData.triggers = triggerNames.uniq().sort();
    for (nickName in hostNames)
      parsedData.hosts[nickName] = hostNames[nickName].uniq().sort();

    return parsedData;
  }

  function setupFilterValues(servers, query) {
    if (!servers && rawData && rawData.servers)
      servers = rawData.servers;

    if (!query)
      query = self.lastQuery ? self.lastQuery : self.baseQuery;

    self.setupHostFilters(servers, query);

    if ("minimumSeverity" in query)
      $("#select-severity").val(query.minimumSeverity);
    if ("status" in query)
      $("#select-status").val(query.status);
  }

  function setLoading(loading) {
    if (loading) {
      $("#select-severity").attr("disabled", "disabled");
      $("#select-status").attr("disabled", "disabled");
      $("#select-server").attr("disabled", "disabled");
      $("#select-host").attr("disabled", "disabled");
    } else {
      $("#select-severity").removeAttr("disabled");
      $("#select-status").removeAttr("disabled");
      $("#select-server").removeAttr("disabled");
      if ($("#select-host option").length > 1)
        $("#select-host").removeAttr("disabled");
    }
  }

  function drawTableHeader(parsedData) {
    var serverName, hostNames, hostName;
    var x, serversRow, hostsRow;

    serversRow = "<tr><th></th>";
    hostsRow = "<tr><th></th>";
    for (var nickName in parsedData.hosts) {
      hostNames = parsedData.hosts[nickName];
      serversRow += "<th style='text-align: center' colspan='" +
        hostNames.length + "'>" + escapeHTML(nickName) + "</th>";
      for (x = 0; x < hostNames.length; ++x) {
        hostName  = hostNames[x];
        hostsRow += "<th>" + escapeHTML(hostName) + "</th>";
      }
    }
    serversRow += "</tr>";
    hostsRow += "</tr>";

    return serversRow + hostsRow;
  }

  function drawTableBody(parsedData) {
    var triggerName, serverName, hostNames, hostName, trigger, html;
    var x, y;

    html = "";
    for (y = 0; y < parsedData.triggers.length; ++y) {
      triggerName = parsedData.triggers[y];
      html += "<tr>";
      html += "<th>" + escapeHTML(triggerName) + "</th>";
      for (serverName in parsedData.hosts) {
        hostNames = parsedData.hosts[serverName];
        for (x = 0; x < hostNames.length; ++x) {
          hostName  = hostNames[x];
          trigger = parsedData.values[serverName][hostName][triggerName];
          if (trigger) {
            switch (trigger["status"]) {
            case 1:
              html += "<td class='severity" +
                escapeHTML(trigger["severity"]) + "'>&nbsp;</td>";
              break;
            case 0:
              html += "<td class='healthy'>&nbsp;</td>";
              break;
            default:
              html += "<td class='unknown'>&nbsp;</td>";
              break;
            }
          } else {
            html += "<td>&nbsp;</td>";
          }
        }
      }
      html += "</tr>";
    }

    return html;
  }

  function drawTableContents(data) {
    $("#table thead").empty();
    $("#table thead").append(drawTableHeader(data));
    $("#table tbody").empty();
    $("#table tbody").append(drawTableBody(data));
  }

  function updateCore(reply, param) {
    rawData = reply;
    parsedData = parseData(rawData, param);
    drawTableContents(parsedData);
    setupFilterValues();
    setupTableSeverityColor();
    setLoading(false);
    self.enableAutoRefresh(load, self.reloadIntervalSeconds);
  }

  function updateFilter(reply) {
    rawData = reply;
    getQuery(false);
    setupFilterValues();
    setupTableSeverityColor();
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

  function getQuery(isEmpty) {
    if (isEmpty) {
      return 'trigger?empty=true';
    }
    var query = $.extend({}, self.baseQuery, {
      minimumSeverity: $("#select-severity").val(),
      status:          $("#select-status").val(),
      limit:           self.baseQuery.limit,
      offset:          self.baseQuery.offset,
    });
    if (self.lastQuery)
      $.extend(query, self.getHostFilterQuery(true));
    self.lastQuery = query;
    return 'trigger?' + $.param(query);
  }

  function resetTriggerPropertyFilter(type) {
    var candidates = triggerPropertyChoices[type];
    var option;
    var currentId = $("#select-" + type).val();

    $("#select-" + type).empty();

    option = $("<option/>", {
      text: "---------",
      value: "-1",
    }).appendTo("#select-" + type);

    $.map(candidates, function(candidate) {
      var option;

      option = $("<option/>", {
        text: candidate.label,
        value: candidate.value
      }).appendTo("#select-" + type);
    });

    $("#select-" + type).val(currentId);
  }

  function load() {
    self.displayUpdateTime();
    self.startConnection(getQuery(), updateCore);
    setLoading(true);
  }

  function setupTableSeverityColor() {
    var severityRanks = self.rawSeverityRankData["SeverityRanks"];
    var severity, color;
    if (!severityRanks)
      return;
    for (var x = 0; x < severityRanks.length; ++x) {
      severity = severityRanks[x].status;
      color = severityRanks[x].color;
      $('td.severity' + severity).css("background-color", color);
    }
    resetTriggerPropertyFilter("severity");
  }

  function loadSeverityRank() {
    var deferred = new $.Deferred();
    new HatoholConnector({
      url: "/severity-rank",
      request: "GET",
      replyCallback: function(reply, parser) {
        var i, severityRanks, rank;
        var choices = triggerPropertyChoices.severity;
        self.rawSeverityRankData = reply;
        self.severityRanksMap = {};
        severityRanks = self.rawSeverityRankData["SeverityRanks"];
        if (severityRanks) {
          for (i = 0; i < severityRanks.length; i++) {
            self.severityRanksMap[severityRanks[i].status] = severityRanks[i];
          }
          for (i = 0; i < choices.length; i++) {
            rank = self.severityRanksMap[choices[i].value];
            if (rank && rank.label)
              choices[i].label = rank.label;
          }
        }
        deferred.resolve();
      },
      parseErrorCallback: function() {
        deferred.reject();
      },
      connectErrorCallback: function() {
        deferred.reject();
      },
    });
    return deferred.promise();
  }
};

OverviewTriggers.prototype = Object.create(HatoholMonitoringView.prototype);
OverviewTriggers.prototype.constructor = OverviewTriggers;
