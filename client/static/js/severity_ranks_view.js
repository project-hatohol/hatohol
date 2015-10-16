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
    var html, severityRankId, status, color, label, isImportant;
    html = "";

    for (var x = 0; x < replyData["SeverityRanks"].length; ++x) {
      severityRankId = replyData["id"][x];
      status = replyData["status"][x];
      color = replyData["color"][x];
      label = replyData["label"][x];
      isImportant = replyData["isImportant"][x];

      html += "<tr>";
      html += "<td>" + severity_choices[Number(status)] + "</td>";
      html += "<td>" + escapeHTML(color) + "</td>";
      html += "<td>" + escapeHTML(label) + "</td>";
      html += "<td class='delete-selector'>";
      html += "<input type='checkbox' class='selectcheckbox' " +
        "severityRankId='" + escapeHTML(severityRankId) + "'></td>";
      html += "</tr>";
    }

    return html;
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
  }

  function load() {
    self.displayUpdateTime();

    self.startConnection(getQuery(self.currentPage), updateCore);
  }
};

SeverityRanksView.prototype = Object.create(HatoholMonitoringView.prototype);
SeverityRanksView.prototype.constructor = SeverityRanksView;
