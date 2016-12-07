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

var DashboardView = function(userProfile) {
  var self = this;

  self.reloadIntervalSeconds = 60;
  self.showToggleAutoRefreshButton();
  self.setupToggleAutoRefreshButtonHandler(load, self.reloadIntervalSeconds);

  var severityLabels = {
    // Status: Label
    0: gettext("Not classified"),
    1: gettext("Information"),
    2: gettext("Warning"),
    3: gettext("Average"),
    4: gettext("High"),
    5: gettext("Disaster")
  };

  // call the constructor of the super class
  HatoholMonitoringView.apply(this, [userProfile]);

  load();

  $('button.latest-button').click(function() {
    load();
  });

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

      for (y = 0; y < serverStatus["systemStatus"].length; ++y) {
        systemStatus = serverStatus["systemStatus"][y];
        groupId = systemStatus["hostgroupId"];
        if (!(groupId in parsedData[serverId]))
          parsedData[serverId][groupId] = {};
        severity = systemStatus["severity"];
        numberOfTriggers = systemStatus["numberOfTriggers"];
        parsedData[serverId][groupId][severity] = numberOfTriggers;
      }
    }

    return parsedData;
  }

  function buildProgress(bad, total) {
    var html = "";
    var bad_ratio;

    bad_ratio = (100 * bad / total) << 0;

    html += "<div class='progress'>";
    html += "<div class='progress-bar progress-bar-success' style='width:" +
      (100 - bad_ratio) + "%;'></div>";
    html += "<div class='progress-bar progress-bar-danger'  style='width:" +
      bad_ratio  + "%;'></div>";
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
      html += "<td rowspan='2'>" + escapeHTML(serverStatus["serverNickname"]) + "</td>";
      html += "<td>" + gettext("Number of hosts [with problem]") + "</td>";
      html += buildRatioColumns(serverStatus["numberOfBadHosts"],
                                serverStatus["numberOfHosts"]);
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
    }

    return html;
  }

  function drawTriggerHead() {
    var html = "";

    html += "<tr>";
    html += "<th>";
    html += gettext("Monitoring Server");
    html += "</th>";
    html += "<th>";
    html += gettext("Group");
    html += "</th>";
    for (var i = Object.keys(severityLabels).length; i > 0; i--) {
      html += "<th>";
      html += severityLabels[i-1];
      html += "</th>";
    }
    html += "</tr>";
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
        if (y === 0) {
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
        if (y === 0) {
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
    $("#tblTrigger thead").empty();
    $("#tblTrigger thead").append(drawTriggerHead());
    $("#tblTrigger tbody").empty();
    $("#tblTrigger tbody").append(drawTriggerBody(rawData, parsedData));
    $("#tblHost tbody").empty();
    $("#tblHost tbody").append(drawHostBody(rawData, parsedData));

    self.displayUpdateTime();
    self.enableAutoRefresh(load, self.reloadIntervalSeconds);
  }

  function drawLogSearchBody(logSearchSystems) {
    var html = "";

    logSearchSystems.forEach(function(logSearchSystem) {
      html += "<tr>";
      html += "<td>";
      html += "<form class='form-inline logSearchForm'>";
      html += "<div class='input-group'>";
      html += "<input type='hidden' name='type' value='" +
        escapeHTML(logSearchSystem.type) + "'>";
      html += "<input type='hidden' name='baseURL' value='" +
        escapeHTML(logSearchSystem.base_url) + "'>";
      html += "<input type='text' name='query' class='form-control'>";
      html += "<span class='input-group-btn'>";
      html += "<button type='submit' class='btn btn-default'>";
      html += "<span class='glyphicon glyphicon-search'></span>";
      html += "</button>";
      html += "</span>";
      html += "</div>";
      html += "</form>";
      html += "</td>";
      html += "<td>";
      html += "<a href='" + escapeHTML(logSearchSystem.base_url) + "' target='_blank'>";
      html += escapeHTML(logSearchSystem.base_url);
      html += "</a>";
      html += "</td>";
      html += "</tr>";
    });

    return html;
  }

  function searchLog(type, baseURL, query) {
    if (!query) {
      return;
    }

    if (type == "groonga") {
      window.open(baseURL + '?query=' + escape(query));
    }
  }

  function updateLogSearch(reply) {
    var logSearchSystems = reply;

    if (logSearchSystems.length > 0) {
      $("#logSearchPanel").show();
      $("#tblLogSearch tbody").empty();
      $("#tblLogSearch tbody").append(drawLogSearchBody(logSearchSystems));
      $(".logSearchForm").submit(function(event) {
        var $form = $(event.target);
        var type =  $form.find("input[name='type']").val();
        var baseURL = $form.find("input[name='baseURL']").val();
        var query = $form.find("input[name='query']").val();
        searchLog(type, baseURL, query);
        return false;
      });
    } else {
      $("#logSearchPanel").hide();
    }
  }

  function load() {
    self.startConnection('severity-rank', loadSeverityLabel);
    self.startConnection('overview', updateCore);
    self.startConnection('log-search-systems/', updateLogSearch, null,
                         {
                           pathPrefix: "",
                         });
  }

  function loadSeverityLabel(reply) {
    var severityRanks = reply["SeverityRanks"];
    if (severityRanks) {
      for (var i = 0; i < severityRanks.length; i++) {
        if (severityRanks[i].label)
          severityLabels[severityRanks[i].status] = severityRanks[i].label;
      }
    }
  }
};

DashboardView.prototype = Object.create(HatoholMonitoringView.prototype);
DashboardView.prototype.constructor = DashboardView;
