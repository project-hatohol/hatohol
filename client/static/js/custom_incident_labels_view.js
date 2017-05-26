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

var CustomIncidentLabelsView = function(userProfile) {
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
  function getDefaultLabel(code) {
    var defaultLabels = {
      "NONE":        pgettext("Incident", "NONE"),
      "HOLD":        pgettext("Incident", "HOLD"),
      "IN PROGRESS": pgettext("Incident", "IN PROGRESS"),
      "DONE":        pgettext("Incident", "DONE"),
    };
    if (defaultLabels[code])
      return defaultLabels[code];
    return "";
  }

  function drawTableBody(replyData) {
    var html, customIncidentStatus, customIncidentStatusId, code, label;
    html = "";

    for (var x = 0; x < replyData.CustomIncidentStatuses.length; ++x) {
      customIncidentStatus = replyData.CustomIncidentStatuses[x];
      customIncidentStatusId = customIncidentStatus.id;
      code = customIncidentStatus.code;
      label = customIncidentStatus.label;

      html += "<tr>";
      html += "<td id='custom-incident-status-code" + escapeHTML(customIncidentStatusId) + "'>" +
        escapeHTML(code) + "</td>";
      html += "<td><input type=\"text\" id='custom-incident-status-label" +
        escapeHTML(customIncidentStatusId) + "'" +
        " contenteditable='true' " +
        " placeholder='" + getDefaultLabel(code) + "' value='" +
        escapeHTML(label) + "'></td>";
      html += "</tr>";
    }

    return html;
  }

  function makeQueryData(customIncidentStatusId) {
    var queryData = {};
    queryData.id = customIncidentStatusId;
    queryData.code = $("#custom-incident-status-code" + customIncidentStatusId).text();
    queryData.label = $("#custom-incident-status-label" + customIncidentStatusId).val();
    return queryData;
  }

  function saveCustomIncidentLabel(status) {
    var deferred = new $.Deferred();
    var url = "/custom-incident-status";
    url += "/" + status;
    new HatoholConnector({
      url: url,
      request: "POST",
      data: makeQueryData(status),
      replyCallback: function() {
        deferred.resolve();
      },
      parseErrorCallback: function() {
        deferred.reject();
      },
      connectErrorCallback: function() {
        deferred.reject();
      },
    });
    return deferred.promise();
  }

  function saveCustomIncidentLabels() {
    var CustomIncidentStatuses = rawData.CustomIncidentStatuses;
    var promise, promises = [];

    // TODO: Save only modified labels

    $.map(CustomIncidentStatuses, function(rank, i) {
      promise = saveCustomIncidentLabel(rank.id);
      promises.push(promise);
    });

    hatoholInfoMsgBox(gettext("Saving..."));

    $.when.apply($, promises)
      .done(function() {
        hatoholInfoMsgBox(gettext("Succeeded to save."));
        self.startConnection('custom-incident-status', updateCore);
      })
      .fail(function() {
        hatoholInfoMsgBox(gettext("Failed to save!"));
        self.startConnection('custom-incident-status', updateCore);
      });
  }

  function setupApplyButton(reply) {
    if (!userProfile.hasFlag(hatohol.OPPRVLG_UPDATE_INCIDENT_SETTING))
      return;

    $("#save-custom-incident-labels").show();
    $("#save-custom-incident-labels").click(function() {
      saveCustomIncidentLabels();
    });
    $("#cancel-custom-incident-labels").show();
    $("#cancel-custom-incident-labels").click(function() {
      load();
      hatoholInfoMsgBox(gettext("Input data has reset."));
    });
  }

  function drawTableContents(data) {
    $("#table tbody").empty();
    $("#table tbody").append(drawTableBody(data));
  }

  function getQuery() {
    return 'custom-incident-status';
  }

  function updateCore(reply) {
    rawData = reply;

    drawTableContents(rawData);
    setupApplyButton(rawData);
  }

  function load() {
    self.displayUpdateTime();

    self.startConnection(getQuery(), updateCore);
  }
};

CustomIncidentLabelsView.prototype = Object.create(HatoholMonitoringView.prototype);
CustomIncidentLabelsView.prototype.constructor = CustomIncidentLabelsView;
