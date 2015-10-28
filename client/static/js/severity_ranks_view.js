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
  self.targetStatus = -1;
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
      html += "<td id='severity-rank-status" + escapeHTML(status) +"'>" +
        severity_choices[Number(status)] + "</td>";
      html += "<td id='severity-rank-color" + escapeHTML(status) + "'" +
        " style='background-color: " + escapeHTML(color) + "; display: none;'>" +
        escapeHTML(color) + "</td>";
      html += "<td id='severity-rank-label" + escapeHTML(status) +"'" +
        "style='display: none;'>" +
        escapeHTML(label) + "</td>";
      html += "<td class='delete-selector'>";
      html += "<input type='checkbox' id='severity-rank-checkbox" +
        escapeHTML(status) +"'";
      if (asImportant) {
        html += "checked='checked'";
      }
      html += " class='selectcheckbox' " +
        "severityRankStatus='" + escapeHTML(status) + "'></td>";

      html += "<td class='edit-severity-rank-setting-column' style='display: none;'>";
      html += "<input id='edit-severity-rank-setting" + escapeHTML(status) + "'";
      html+= "  type='button' class='btn btn-default'";
      html += "  severityStatus='" + escapeHTML(status) + "'";
      html += "  value='" + gettext("APPLY") + "' />";
      html += "</td>";

      html += "</tr>";
    }

    return html;
  }

  function makeQueryData(status) {
    var queryData = {};
    queryData.status = status;
    queryData.color = $("#severity-rank-color" + status).text();
    queryData.label = $("#severity-rank-label" + status).text();
    queryData.asImportant = $("#severity-rank-checkbox" + status).is(":checked");
    return queryData;
  }

  function setupApplyButtons(reply) {
    var i, status, severityRanks = reply["SeverityRanks"];

    for (i = 0; i < severityRanks.length; ++i) {
      status = "#edit-severity-rank-setting" + severityRanks[i].status;
      $(status).click(function() {
        var severityStatus = this.getAttribute("severityStatus");
        var url = "/severity-rank";
        url += "/" + severityStatus;
        self.targetStatus = severityStatus;
        new HatoholConnector({
          url: url,
          request: "POST",
          data: makeQueryData(self.targetStatus),
          replyCallback: function() {
            // nothing to do
          },
          parseErrorCallback: hatoholErrorMsgBoxForParser,
          completionCallback: function() {
            self.startConnection('severity-rank', updateCore);
          },
        });
      });
    }

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

    self.startConnection(getQuery(), updateCore);
  }
};

SeverityRanksView.prototype = Object.create(HatoholMonitoringView.prototype);
SeverityRanksView.prototype.constructor = SeverityRanksView;
