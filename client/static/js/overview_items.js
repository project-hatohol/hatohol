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

var OverviewItems = function(userProfile) {
  var self = this;
  var rawData, parsedData;

  // call the constructor of the super class
  HatoholMonitoringView.apply(userProfile);

  self.startConnection('item', updateCore);

  $("#select-server").change(function() {
    var serverName = $("#select-server").val();
    self.setCandidate($("#select-host"), parsedData.hosts[serverName]);
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
    self.updateScreen(rawData, updateCore, s);
  });

  function parseData(replyData) {
    var parsedData = {};
    var serverName, hostName, itemName;
    var serverNames, itemNames, hostNames;
    var server, item;
    var x;

    parsedData.hosts  = {};
    parsedData.values = {};

    serverNames = [];
    itemNames = [];
    hostNames = {};

    for (x = 0; x < replyData["items"].length; ++x) {
      item = replyData["items"][x];
      server = replyData["servers"][item["serverId"]];
      serverName = getServerName(server, item["serverId"]);
      hostName   = getHostName(server, item["hostId"]);
      itemName   = item["brief"];

      serverNames.push(serverName);
      itemNames.push(itemName);
      if (!hostNames[serverName])
        hostNames[serverName] = [];
      hostNames[serverName].push(hostName);

      if (!parsedData.values[serverName])
        parsedData.values[serverName] = {};
      if (!parsedData.values[serverName][hostName])
        parsedData.values[serverName][hostName] = {};

      if (!parsedData.values[serverName][hostName][itemName])
        parsedData.values[serverName][hostName][itemName] = item;
    }

    parsedData.servers = serverNames.uniq().sort();
    parsedData.items   = itemNames.uniq().sort();
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
        hostName = hostNames[x];
        if (targetHostName && hostName != targetHostName)
          continue;

        hostsRow += "<th>" + escapeHTML(hostName) + "</th>";
      }
    }
    hostsRow += "</tr>";
    hostsRow += "</tr>";

    return serversRow + hostsRow;
  }

  function drawTableBody(parsedData) {
    var serverName, hostNames, hostName, itemName, item, html;
    var x, y;
    var targetServerName = getTargetServerName();
    var targetHostName = getTargetHostName();

    html = "";
    for (y = 0; y < parsedData.items.length; ++y) {
      itemName = parsedData.items[y];
      html += "<tr>";
      html += "<th>" + escapeHTML(itemName) + "</th>";
      for (serverName in parsedData.hosts) {
        if (targetServerName && serverName != targetServerName)
          continue;

        hostNames = parsedData.hosts[serverName];
        for (x = 0; x < hostNames.length; ++x) {
          hostName = hostNames[x];
          if (targetHostName && hostName != targetHostName)
            continue;

          item = parsedData.values[serverName][hostName][itemName];
          if (item && item["lastValue"] != undefined) {
            html += "<td>" + escapeHTML(item["lastValue"]) + "</td>";
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

  function updateCore(reply) {
    rawData = reply;
    parsedData = parseData(reply);
    self.setCandidate($("#select-server"), parsedData.servers);
    self.setCandidate($("#select-host"));
    drawTableContents(parsedData);
  }
};

OverviewItems.prototype = Object.create(HatoholMonitoringView.prototype);
OverviewItems.prototype.constructor = OverviewItems;
