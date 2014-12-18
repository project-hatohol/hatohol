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

var IncidentSettingsView = function(userProfile) {
  //
  // Variables
  //
  var self = this;
  self.incidentSettingsData = null;
  self.incidentTrackersData = null;
  self.incidentTrackersMap = null;

  // call the constructor of the super class
  HatoholMonitoringView.apply(this, [userProfile]);

  //
  // main code
  //
  load();

  //
  // Main view
  //
  $("#table").stupidtable();
  $("#table").bind('aftertablesort', function(event, data) {
    var th = $(this).find("th");
    th.find("i.sort").remove();
    var icon = data.direction === "asc" ? "up" : "down";
    th.eq(data.column).append("<i class='sort glyphicon glyphicon-arrow-" + icon +"'></i>");
  });

  $("#add-incident-setting-button").click(function() {
    var incidentTrackers = self.incidentTrackersData.incidentTrackers;
    new HatoholAddActionDialog(load, incidentTrackers);
  });

  $("#delete-incident-setting-button").click(function() {
    var msg = gettext("Do you delete the selected items ?");
    hatoholNoYesMsgBox(msg, deleteActions);
  });

  $("#edit-incident-trackers-button").click(function() {
    new HatoholIncidentTrackersEditor({
      changedCallback: load,
    });
  });

  //
  // Commonly used functions from a dialog.
  //
  function parseResult(data) {
    var msg;
    var malformed = false;
    if (data.result == undefined)
      malformed = true;
    if (!malformed && !data.result && data.message == undefined)
      malformed = true;
    if (malformed) {
      msg = "The returned content is malformed: " +
        "Not found 'result' or 'message'.\n" + JSON.stringify(data);
      hatoholErrorMsgBox(msg);
      return false;
    }
    if (!data.result) {
      msg = "Failed:\n" + data.message;
      hatoholErrorMsgBox(msg);
      return false;
    }

    if (data.id == undefined) {
      msg = "The returned content is malformed: " +
        "'result' is true, however, 'id' is missing.\n" +
        JSON.stringify(data);
      hatoholErrorMsgBox(msg);
      return false;
    }
    return true;
  }

  function deleteActions() {
    $(this).dialog("close");
    var checkboxes = $(".selectcheckbox");
    var deleteList = [], i;
    for (i = 0; i < checkboxes.length; i++) {
      if (checkboxes[i].checked)
        deleteList.push(checkboxes[i].getAttribute("actionId"));
    }
    new HatoholItemRemover({
      id: deleteList,
      type: "action",
      completionCallback: function() {
        load();
      }
    });
    hatoholInfoMsgBox(gettext("Deleting..."));
  }

  //
  // Functions for make the main view.
  //
  function makeSeverityCompTypeLabel(compType) {
    switch(compType) {
    case hatohol.CMP_EQ:
      return "=";
    case hatohol.CMP_EQ_GT:
      return ">=";
    default:
      return "INVALID: " + compType;
    }
  }

  //
  // parser of received json data
  //
  function getServerNameFromAction(actionsPkt, actionDef) {
    var serverId = actionDef["serverId"];
    if (!serverId)
      return "ANY";
    return getServerName(actionsPkt["servers"][serverId], serverId);
  }

  function getHostgroupNameFromAction(actionsPkt, actionDef) {
    var serverId = actionDef["serverId"];
    var hostgroupId = actionDef["hostgroupId"];
    if (!hostgroupId)
      return "ANY";
    return getHostgroupName(actionsPkt["servers"][serverId], hostgroupId);
  }

  //
  // callback function from the base template
  //
  function drawTableBody(actionsPkt) {
    var x;
    var klass, server, host;

    var s = "";
    for (x = 0; x < actionsPkt["actions"].length; ++x) {
      var actionDef = actionsPkt["actions"][x];
      s += "<tr>";
      s += "<td class='delete-selector'>";
      s += "<input type='checkbox' class='selectcheckbox' " +
        "actionId='" + escapeHTML(actionDef.actionId) + "'></td>";
      s += "<td>" + escapeHTML(actionDef.actionId) + "</td>";

      var serverName = getServerNameFromAction(actionsPkt, actionDef);
      s += "<td>" + escapeHTML(serverName) + "</td>";

      var hostgroupName = getHostgroupNameFromAction(actionsPkt, actionDef);
      s += "<td>" + escapeHTML(hostgroupName) + "</td>";

      var triggerStatus = actionDef.triggerStatus;
      var triggerStatusLabel = "ANY";
      if (triggerStatus != undefined)
        triggerStatusLabel = makeTriggerStatusLabel(triggerStatus);
      s += "<td>" + triggerStatusLabel + "</td>";

      var triggerSeverity = actionDef.triggerSeverity;
      var severityLabel = "ANY";
      if (triggerSeverity != undefined)
        severityLabel = makeSeverityLabel(triggerSeverity);

      var severityCompType = actionDef.triggerSeverityComparatorType;
      var severityCompLabel = "";
      if (triggerSeverity != undefined)
        severityCompLabel = makeSeverityCompTypeLabel(severityCompType);

      s += "<td>" + severityCompLabel + " " + severityLabel + "</td>";

      var command = actionDef.command;
      var incidentTracker = self.incidentTrackersMap[actionDef.command];
      s += "<td>";
      if (incidentTracker) {
	s += incidentTracker.nickname;
        s += " (" + gettext("Project: ") + incidentTracker.projectId;
        if (incidentTracker.trackerId) {
          s += ", " + gettext("Tracker: ") + incidentTracker.trackerId;
        }
        s += ")";
      } else {
        s += gettext("Unknown");
      }
      s += "</td>";

      s += "</tr>";
    }

    return s;
  }

  function parseIncidentTrackers(incidentTrackersData) {
    var incidentTrackers = incidentTrackersData.incidentTrackers;
    var i, incidentTrackersMap = {};
    for (i = 0; i < incidentTrackers.length; i++)
      incidentTrackersMap[incidentTrackers[i].id] = incidentTrackers[i];
    return incidentTrackersMap;
  }

  function onGotIncidentTrackers(incidentTrackersData) {
    self.incidentTrackersData = incidentTrackersData;
    self.incidentTrackersMap = parseIncidentTrackers(self.incidentTrackersData);
    $("#table tbody").empty();
    $("#table tbody").append(drawTableBody(self.incidentSettingsData));
    self.setupCheckboxForDelete($("#delete-incident-setting-button"));
  }

  function onGotIncidentSettings(incidentSettingsData) {
    self.incidentSettingsData = incidentSettingsData;
    self.startConnection("incident-trackers", onGotIncidentTrackers);
  }

  function getQuery() {
    var query = {
      type: hatohol.ACTION_INCIDENT_SENDER,
    };
    return 'action?' + $.param(query);
  };

  function load() {
    self.startConnection(getQuery(), onGotIncidentSettings);
  }
};

IncidentSettingsView.prototype = Object.create(HatoholMonitoringView.prototype);
IncidentSettingsView.prototype.constructor = IncidentSettingsView;
