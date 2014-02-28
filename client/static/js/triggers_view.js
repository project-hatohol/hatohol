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

var TriggersView = function(userProfile) {
  var self = this;
  var rawData;

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

  $("#select-server").change(function() {
    self.setHostFilterCandidates(rawData["servers"]);
    drawTableContents(rawData);
  });
  $("#select-host").change(function() {
    drawTableContents(rawData);
  });
  $("#select-severity").change(function() {
    drawTableContents(rawData);
  });
  $("#select-status").change(function() {
    drawTableContents(rawData);
  });

  function drawTableBody(replyData) {
    var serverName, hostName, clock, status, severity;
    var html, server, trigger;
    var x;
    var targetServerId = self.getTargetServerId();
    var targetHostId = self.getTargetHostId();
    var minimumSeverity = $("#select-severity").val();
    var targetStatus = $("#select-status").val();

    html = "";
    for (x = 0; x < replyData["triggers"].length; ++x) {
      trigger = replyData["triggers"][x];
      if (trigger["severity"] < minimumSeverity)
        continue;
      if (targetStatus >= 0 && trigger["status"] != targetStatus)
        continue;

      var serverId = trigger["serverId"];
      var hostId = trigger["hostId"];
      server     = replyData["servers"][serverId];
      serverName = getServerName(server, serverId);
      hostName   = getHostName(server, hostId);
      clock      = trigger["lastChangeTime"];
      status     = trigger["status"];
      severity   = trigger["severity"];

      if (targetServerId && serverId != targetServerId)
        continue;
      if (targetHostId && hostId != targetHostId)
        continue;

      html += "<tr><td>" + escapeHTML(serverName) + "</td>";
      html += "<td class='severity" + escapeHTML(severity) +
        "' data-sort-value='" + escapeHTML(severity) + "'>" +
        severity_choices[Number(severity)] + "</td>";
      html += "<td class='status" + escapeHTML(status) +
        "' data-sort-value='" + escapeHTML(status) + "'>" +
        status_choices[Number(status)] + "</td>";
      html += "<td data-sort-value='" + escapeHTML(clock) + "'>" +
        formatDate(clock) + "</td>";
      /* Not supported yet
      html += "<td>" + "unsupported" + "</td>";
      html += "<td>" + "unsupported" + "</td>";
      */
      html += "<td>" + escapeHTML(hostName) + "</td>";
      html += "<td>" + escapeHTML(trigger["brief"]) + "</td>";
      // Not supported yet
      //html += "<td>" + "unsupported" + "</td>";
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
    self.setHostFilterCandidates(rawData["servers"]);

    drawTableContents(rawData);
    self.setAutoReload(load, self.reloadIntervalSeconds);
  }

  function load() {
    self.startConnection('trigger', updateCore);
  }
};

TriggersView.prototype = Object.create(HatoholMonitoringView.prototype);
TriggersView.prototype.constructor = TriggersView;
