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

var DashboardView = function(userProfile) {
  var self = this;

  self.reloadIntervalSeconds = 60;

  // call the constructor of the super class
  HatoholMonitoringView.apply(this, [userProfile]);

  load();

  function parseData(replyData) {
    var parsedData = {};
    var serverStatus, hostStatus, systemStatus;
    var x, y;
    var serverId, groupId, severity, numberOfTriggers;

    for (x = 0; x < replyData["serverStatus"].length; ++x) {
      serverStatus = replyData["serverStatus"][x];
      serverId = serverStatus["serverId"];
      parsedData[serverId] = {};

      parsedData[serverId]["numberOfHostgroups"] = 0;
      for (y in serverStatus["hostgroups"])
        parsedData[serverId]["numberOfHostgroups"] += 1;

      parsedData[serverId]["problem"] = 0;
      for (y = 0; y < serverStatus["systemStatus"].length; ++y) {
        systemStatus = serverStatus["systemStatus"][y];
        groupId = systemStatus["hostgroupId"];
        if (!(groupId in parsedData[serverId]))
          parsedData[serverId][groupId] = {};
        severity = systemStatus["severity"];
        numberOfTriggers = systemStatus["numberOfTriggers"];
        parsedData[serverId][groupId][severity] = numberOfTriggers;
        parsedData[serverId]["problem"] += numberOfTriggers;
      }
    }

    return parsedData;
  }

  function buildProgress(bad, total) {
    var html = "";
    var bad_ratio;

    bad_ratio = (100 * bad / total) << 0;

    html += "<div class='progress'>";
    html += "<div class='progress-bar progress-bar-success' style='width:" + (100 - bad_ratio) + "%;'></div>";
    html += "<div class='progress-bar progress-bar-danger'  style='width:" +        bad_ratio  + "%;'></div>";
    html += "</div>";

    return html;
  }

  function buildRatioColumns(numerator, denominator) {
    var html = "";
    html += "<td>" + numerator + " / " + denominator + "</td>";
    html += "<td>" + buildProgress(numerator, denominator) + "</td>";
    return html;
  }

  function drawServerBody(replyData, parsedData) {
    var html = "";
    var serverStatus;
    var serverStatuses;
    var x;
    var serverId;

    serverStatuses = replyData["serverStatus"];

    html += "<tr>";
    html += "<td colspan='2'>" + gettext("Number of servers [With problem]") + "</td>";
    html += buildRatioColumns(replyData["numberOfBadServers"],
                              replyData["numberOfServers"]);
    html += "</tr>";

    for (x = 0; x < serverStatuses.length; ++x) {
      serverStatus = serverStatuses[x];
      serverId = serverStatus["serverId"];
      html += "<tr>";
      html += "<td rowspan='4'>" + escapeHTML(serverStatus["serverNickname"]) + "</td>";
      html += "<td>" + gettext("Number of hosts [with problem]") + "</td>";
      html += buildRatioColumns(serverStatus["numberOfBadHosts"],
                                serverStatus["numberOfHosts"]);
      html += "</tr>";
      html += "<tr>";
      html += "<td>" + gettext("Number of items") + "</td>";
      html += "<td>" + escapeHTML(serverStatus["numberOfItems"]) + "</td>";
      html += "<td></td>";
      html += "</tr>";
      html += "<tr>";
      html += "<td>" + gettext("Number of triggers [with problem]") + "</td>";
      html += buildRatioColumns(serverStatus["numberOfBadTriggers"],
                                serverStatus["numberOfTriggers"]);
      html += "</tr>";
      /* Not implemeneted yet
      html += "<tr>";
      html += "<td>" + gettext("Number of users [Online]") + "</td>";
      html += buildRatioColumns(serverStatus["numberOfOnlineUsers"],
                                serverStatus["numberOfUsers"]);
      html += "</tr>";
      */
      html += "<tr>";
      html += "<td>" + gettext("New values per second") + "</td>";
      html += "<td>" + escapeHTML(serverStatus["numberOfMonitoredItemsPerSecond"]) + "</td>";
      html += "<td></td>";
      html += "</tr>";
    }

    return html;
  }

  function drawTriggerBody(replyData, parsedData) {
    var html = "";
    var x, y, severity;
    var serverId, groupId, numberOfTriggers;

    for (x = 0; x < replyData["serverStatus"].length; ++x) {
      serverId = replyData["serverStatus"][x]["serverId"];
      html += "<tr>";
      y = 0;
      for (groupId in replyData["serverStatus"][x]["hostgroups"]) {
        html += "<tr>"; // ==============  start of a row ================
        if (y == 0) {
          html += "<td rowspan='" + escapeHTML(parsedData[serverId]["numberOfHostgroups"]) + "'>";
          html += escapeHTML(replyData["serverStatus"][x]["serverNickname"]);
          html += "</td>";
        }
        html += "<td>";
        html += escapeHTML(replyData["serverStatus"][x]["hostgroups"][groupId]["name"]);
        html += "</td>";
        for (severity = 5; severity >= 0; --severity) {
          numberOfTriggers = parsedData[serverId][groupId][severity];
          if (isNaN(numberOfTriggers))
            numberOfTriggers = 0;
          html += "<td>" + escapeHTML(numberOfTriggers) + "</td>";
        }
        html += "</tr>"; // ============== end of a row ================
        ++y;
      }
      html += "</tr>";
    }

    return html;
  }

  function drawHostBody(replyData, parsedData) {
    var html = "";
    var serverStatus, hostStatuses, hostStatus;
    var x, y;

    for (x = 0; x < replyData["serverStatus"].length; ++x) {
      serverStatus = replyData["serverStatus"][x];
      hostStatuses = replyData["serverStatus"][x]["hostStatus"];
      html += "<tr>";
      for (y = 0; y < hostStatuses.length; ++y) {
        hostStatus = hostStatuses[y];
        html += "<tr>"; // ==============  start of a row ================
        if (y == 0) {
          html += "<td rowspan='" + hostStatuses.length + "'>";
          html += escapeHTML(replyData["serverStatus"][x]["serverNickname"]);
          html += "</td>";
        }
        html += "<td>";
        html += escapeHTML(serverStatus["hostgroups"][hostStatus["hostgroupId"]]["name"]);
        html += "</td>";
        html += "<td>";
        html += escapeHTML(hostStatus["numberOfGoodHosts"]);
        html += "</td>";
        html += "<td>";
        html += escapeHTML(hostStatus["numberOfBadHosts"]);
        html += "</td>";
        html += "<td>";
        html += escapeHTML(hostStatus["numberOfGoodHosts"] + hostStatus["numberOfBadHosts"]);
        html += "</td>";
        html += "</tr>"; // ============== end of a row ================
      }
      html += "</tr>";
    }

    return html;
  }

  function updateCore(reply) {
    var rawData = reply;
    var parsedData = parseData(rawData);

    $("#tblServer tbody").empty();
    $("#tblServer tbody").append(drawServerBody(rawData, parsedData));
    $("#tblTrigger tbody").empty();
    $("#tblTrigger tbody").append(drawTriggerBody(rawData, parsedData));
    $("#tblHost tbody").empty();
    $("#tblHost tbody").append(drawHostBody(rawData, parsedData));

    self.setAutoReload(load, self.reloadIntervalSeconds);
  }

  function load() {
    self.startConnection('overview', updateCore);
  }
};

DashboardView.prototype = Object.create(HatoholMonitoringView.prototype);
DashboardView.prototype.constructor = DashboardView;
