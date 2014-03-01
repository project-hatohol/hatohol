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

var OverviewTriggers = function(userProfile) {
  var self = this;
  var rawData, parsedData;

  self.reloadIntervalSeconds = 60;

  // call the constructor of the super class
  HatoholMonitoringView.apply(userProfile);

  load();

  $("#select-server").change(function() {
    self.setHostFilterCandidates(rawData["servers"]);
    load();
  });
  $("#select-group").change(function() {
    load();
  });
  $("#select-host").change(function() {
    load();
  });
  $("#select-severity").change(function() {
    load();
  });
  $("#select-status").change(function() {
    load();
  });

  function parseData(replyData, minimum) {
    var parsedData = {};
    var serverName, hostName, triggerName;
    var serverNames, triggerNames, hostNames;
    var server, trigger;
    var x;
    var targetSeverity = $("#select-severity").val();
    var targetStatus = $("#select-status").val();

    parsedData.hosts  = {};
    parsedData.values = {};

    serverNames = [];
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
      serverName  = getServerName(server, serverId);
      hostName    = getHostName(server, hostId);
      triggerName = trigger["brief"];

      serverNames.push(serverName);
      triggerNames.push(triggerName);
      if (!hostNames[serverName])
        hostNames[serverName] = [];
      hostNames[serverName].push(hostName);

      if (!parsedData.values[serverName])
        parsedData.values[serverName] = {};
      if (!parsedData.values[serverName][hostName])
        parsedData.values[serverName][hostName] = {};

      if (!parsedData.values[serverName][hostName][triggerName])
        parsedData.values[serverName][hostName][triggerName] = trigger;
    }

    parsedData.servers  = serverNames.uniq().sort();
    parsedData.triggers = triggerNames.uniq().sort();
    for (serverName in hostNames)
      parsedData.hosts[serverName] = hostNames[serverName].uniq().sort();

    return parsedData;
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
    for (serverName in parsedData.hosts) {
      hostNames = parsedData.hosts[serverName];
      serversRow += "<th style='text-align: center' colspan='" +
        hostNames.length + "'>" + escapeHTML(serverName) + "</th>";
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
    self.setServerFilterCandidates(rawData["servers"]);
    self.setHostFilterCandidates(rawData["servers"]);
    drawTableContents(parsedData);
    setLoading(false);
    self.setAutoReload(load, self.reloadIntervalSeconds);
  }

  function getQuery() {
    var serverId = self.getTargetServerId();
    var hostId = self.getTargetHostId();
    var query = {
      minimumSeverity: $("#select-severity").val(),
      status:          $("#select-status").val(),
      maximumNumber:   0,
      offset:          0
    };
    if (serverId)
      query.serverId = serverId;
    if (hostId)
      query.hostId = hostId;
    return 'trigger?' + $.param(query);
  };

  function load() {
    self.startConnection(getQuery(), updateCore);
    setLoading(true);
  }
};

OverviewTriggers.prototype = Object.create(HatoholMonitoringView.prototype);
OverviewTriggers.prototype.constructor = OverviewTriggers;
