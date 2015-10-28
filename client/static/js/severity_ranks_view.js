/*
 * Copyright (C) 2015 Project Hatohol
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

var SeverityRanksView = function(userProfile) {
  //
  // Variables
  //
  var self = this;
  var rawData;

  // call the constructor of the super class
  HatoholMonitoringView.apply(this, [userProfile]);

  //
  // main code
  //
  load();

  //
  // Main view
  //
  var severity_choices = [
    gettext("Not classified"),
    gettext("Information"),
    gettext("Warning"),
    gettext("Average"),
    gettext("High"),
    gettext("Disaster")
  ];

  function drawTableBody(replyData) {
    var html, severityRank, severityRankId, status, color, label, asImportant;
    html = "";

    for (var x = 0; x < replyData["SeverityRanks"].length; ++x) {
      severityRank = replyData["SeverityRanks"][x];
      severityRankId = severityRank["id"];
      status = severityRank["status"];
      color = severityRank["color"];
      label = severityRank["label"];
      asImportant = severityRank["asImportant"];

      html += "<tr>";
      html += "<td>" + severity_choices[Number(status)] + "</td>";
      html += "<td style='background-color: " + escapeHTML(color) + "'>" +
        escapeHTML(color) + "</td>";
      html += "<td>" + escapeHTML(label) + "</td>";
      html += "<td class='delete-selector'>";
      html += "<input type='checkbox' ";
      if (asImportant) {
        html += "checked";
      }
      html += " class='selectcheckbox' " +
        "severityRankId='" + escapeHTML(severityRankId) + "'></td>";

      html += "<td class='edit-severity-rank-setting-column' style='display: none;'>";
      html += "<input id='edit-severity-rank-setting" + escapeHTML(severityRankId) + "'";
      html+= "  type='button' class='btn btn-default'";
      html += "  severityRankId='" + escapeHTML(severityRankId) + "'";
      html += "  value='" + gettext("APPLY") + "' />";
      html += "</td>";

      html += "</tr>";
    }

    return html;
  }

  function setupApplyButtons(reply) {
    if (userProfile.hasFlag(hatohol.OPPRVLG_UPDATE_SEVERITY_RANK)) {
      $(".edit-severity-rank-setting-column").show();
    }
  }

  function drawTableContents(data) {
    $("#table tbody").empty();
    $("#table tbody").append(drawTableBody(data));
  }

  function getQuery() {
    return 'severity-rank';
  };

  function updateCore(reply) {
    rawData = reply;

    drawTableContents(rawData);
    setupApplyButtons(rawData);
  }

  function load() {
    self.displayUpdateTime();

    self.startConnection(getQuery(self.currentPage), updateCore);
  }
};

SeverityRanksView.prototype = Object.create(HatoholMonitoringView.prototype);
SeverityRanksView.prototype.constructor = SeverityRanksView;
