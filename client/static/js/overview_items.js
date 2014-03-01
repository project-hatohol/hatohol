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
    // not implemented yet
    load();
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

  function setLoading(loading) {
    if (loading) {
      $("#select-server").attr("disabled", "disabled");
      $("#select-host").attr("disabled", "disabled");
    } else {
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
      serversRow += "<th style='text-align: center' colspan='" + hostNames.length + "'>" + escapeHTML(serverName) + "</th>";
      for (x = 0; x < hostNames.length; ++x) {
        hostName = hostNames[x];
        hostsRow += "<th>" + escapeHTML(hostName) + "</th>";
      }
    }
    hostsRow += "</tr>";
    hostsRow += "</tr>";

    return serversRow + hostsRow;
  }

  function drawTableBody(parsedData) {
    var serverName, hostNames, hostName, itemName, item, html = "";
    var x, y;

    for (y = 0; y < parsedData.items.length; ++y) {
      itemName = parsedData.items[y];
      html += "<tr>";
      html += "<th>" + escapeHTML(itemName) + "</th>";
      for (serverName in parsedData.hosts) {
        hostNames = parsedData.hosts[serverName];
        for (x = 0; x < hostNames.length; ++x) {
          hostName = hostNames[x];
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
      maximumNumber: 0,
      offset:        0
    };
    if (serverId)
      query.serverId = serverId;
    if (hostId)
      query.hostId = hostId;
    return 'item?' + $.param(query);
  };

  function load() {
    self.startConnection(getQuery(), updateCore);
    setLoading(true);
  }
};

OverviewItems.prototype = Object.create(HatoholMonitoringView.prototype);
OverviewItems.prototype.constructor = OverviewItems;
