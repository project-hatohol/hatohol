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
  var rawData, parsedData;

  $("#select-server").change(function() {
    var serverName = $("#select-server").val();
    setCandidate($("#select-host"), parsedData.hosts[serverName]);
    drawTableContents(parsedData);
  });
  $("#select-group").change(function() {
    drawTableContents(parsedData);
  });
  $("#select-host").change(function() {
    drawTableContents(parsedData);
  });
  $("#select-severity").change(function() {
    var s = $("#select-severity").val();
    updateScreen(rawData, updateCore, s);
  });
  $("#select-status").change(function() {
    var s = $("#select-severity").val();
    updateScreen(rawData, updateCore, s);
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

  function getTargetServerName() {
    var name = $("#select-server").val();
    if (name == "---------")
      name = null;
    return name;
  }

  function getTargetHostName() {
    var name = $("#select-host").val();
    if (name == "---------")
      name = null;
    return name;
  }

  function drawTableHeader(parsedData) {
    var serverName, hostNames, hostName;
    var x;
    var serversRow, hostsRow;
    var targetServerName = getTargetServerName();
    var targetHostName = getTargetHostName();

    serversRow = "<tr><th></th>";
    hostsRow = "<tr><th></th>";
    for (serverName in parsedData.hosts) {
      if (targetServerName && serverName != targetServerName)
        continue;

      hostNames = parsedData.hosts[serverName];
      serversRow += "<th style='text-align: center' colspan='" + hostNames.length + "'>" + escapeHTML(serverName) + "</th>";
      for (x = 0; x < hostNames.length; ++x) {
        hostName  = hostNames[x];
        if (targetHostName && hostName != targetHostName)
          continue;

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
    var targetServerName = getTargetServerName();
    var targetHostName = getTargetHostName();

    html = "";
    for (y = 0; y < parsedData.triggers.length; ++y) {
      triggerName = parsedData.triggers[y];
      html += "<tr>";
      html += "<th>" + escapeHTML(triggerName) + "</th>";
      for (serverName in parsedData.hosts) {
        if (targetServerName && serverName != targetServerName)
          continue;

        hostNames = parsedData.hosts[serverName];
        for (x = 0; x < hostNames.length; ++x) {
          hostName  = hostNames[x];
          if (targetHostName && hostName != targetHostName)
            continue;

          trigger = parsedData.values[serverName][hostName][triggerName];
          if (trigger) {
            switch (trigger["status"]) {
            case 1:
              html += "<td class='severity" + escapeHTML(trigger["severity"]) + "'>&nbsp;</td>";
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
    setCandidate($("#select-server"), parsedData.servers);
    setCandidate($("#select-host"));
    drawTableContents(parsedData);
  }

  startConnection('trigger', updateCore, 0);
};
