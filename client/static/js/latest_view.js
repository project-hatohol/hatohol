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

var LatestView = function(userProfile) {
  var self = this;
  var rawData, parsedData;

  self.reloadIntervalSeconds = 60;

  // call the constructor of the super class
  HatoholMonitoringView.apply(userProfile);

  load();

  $("#table").stupidtable();
  $("#table").bind('aftertablesort', function(event, data) {
    var th = $(this).find("th");
    th.find("i.sort").remove();
    var icon = data.direction === "asc" ? "up" : "down";
    th.eq(data.column).append("<i class='sort glyphicon glyphicon-arrow-" + icon +"'></i>");
  });

  $("#select-server, #select-host-group, #select-host").change(function() {
    load();
  });
  $("#select-application").change(function() {
    // will be migrated to server side
    drawTableContents(rawData);
  });

  function parseData(replyData) {
    var parsedData = {};
    var appNames = [];
    var x, item;

    for (x = 0; x < replyData["items"].length; ++x) {
      item = replyData["items"][x];

      if (item["itemGroupName"].length == 0)
        item["itemGroupName"] = "_non_";
      else
        appNames.push(item["itemGroupName"]);
    }
    parsedData.applications = appNames.uniq().sort();

    return parsedData;
  }

  function getTargetAppName() {
    var name = $("#select-application").val();
    if (name == "---------")
      name = null;
    return name;
  }

  function setLoading(loading) {
    if (loading) {
      $("#select-server").attr("disabled", "disabled");
      $("#select-host").attr("disabled", "disabled");
      $("#select-application").attr("disabled", "disabled");
    } else {
      $("#select-server").removeAttr("disabled");
      if ($("#select-host option").length > 1)
        $("#select-host").removeAttr("disabled");
    }
    $("#select-application").removeAttr("disabled");
  }

  function drawTableBody(replyData) {
    var serverName, hostName, clock, appName;
    var html = "", url, server, item, x;
    var targetAppName = getTargetAppName();

    html = "";
    for (x = 0; x < replyData["items"].length; ++x) {
      item       = replyData["items"][x];
      server     = replyData["servers"][item["serverId"]];
      url        = getItemGraphLocation(server, item["id"]);
      serverName = getServerName(server, item["serverId"]);
      hostName   = getHostName(server, item["hostId"]);
      clock      = item["lastValueTime"];
      appName    = item["itemGroupName"];

      if (targetAppName && appName != targetAppName)
        continue;

      html += "<tr><td>" + escapeHTML(serverName) + "</td>";
      html += "<td>" + escapeHTML(hostName) + "</td>";
      html += "<td>" + escapeHTML(appName) + "</td>";
      if (url)
        html += "<td><a href='" + url + "'>" + escapeHTML(item["brief"])  + "</a></td>";
      else
        html += "<td>" + escapeHTML(item["brief"])  + "</td>";
      html += "<td data-sort-value='" + escapeHTML(clock) + "'>" + formatDate(clock) + "</td>";
      html += "<td>" + escapeHTML(item["lastValue"]) + "</td>";
      html += "<td>" + escapeHTML(item["prevValue"]) + "</td>";
      html += "</tr>";
    }

    return html;
  }

  function drawTableContents(rawData) {
    $("#table tbody").empty();
    $("#table tbody").append(drawTableBody(rawData));
  }

  function updateCore(reply) {
    rawData = reply;
    parsedData = parseData(rawData);

    self.setServerFilterCandidates(rawData["servers"]);
    self.setHostFilterCandidates(rawData["servers"]);
    self.setFilterCandidates($("#select-application"), parsedData.applications);

    drawTableContents(rawData);
    setLoading(false);
    self.setAutoReload(load, self.reloadIntervalSeconds);
  }

  function getQuery() {
    var query = {
      maximumNumber: 0,
      offset:        0
    };
    self.addHostQuery(query);
    return 'item?' + $.param(query);
  };

  function load() {
    self.startConnection(getQuery(), updateCore);
    setLoading(true);
  }
};

LatestView.prototype = Object.create(HatoholMonitoringView.prototype);
LatestView.prototype.constructor = LatestView;
