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
  if (userProfile.hasFlag(hatohol.OPPRVLG_DELETE_INCIDENT_SETTING))
    $("#delete-incident-setting-button").show();
  load();

  //
  // Main view
  //
  $("#add-incident-setting-button").click(function() {
    var incidentTrackers = self.incidentTrackersData.incidentTrackers;
    new HatoholAddActionDialog(load, incidentTrackers);
  });

  $("#delete-incident-setting-button").click(function() {
    var msg = gettext("Delete the selected items ?");
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
    if (data.result === undefined)
      malformed = true;
    if (!malformed && !data.result && data.message === undefined)
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

    if (data.id === undefined) {
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
    case hatohol.CMP_INVALID:
      return "";
    default:
      return "INVALID: " + compType;
    }
  }

  //
  // parser of received json data
  //
  function getNickNameFromAction(actionsPkt, actionDef) {
    var serverId = actionDef["serverId"];
    if (!serverId)
      return "ANY";
    return getNickName(actionsPkt["servers"][serverId], serverId);
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
  function drawTableBody(actionsPkt)
  {
    var s = "";
    for (var i = 0; i < actionsPkt["actions"].length; ++i)
    {
      var actionDef = actionsPkt["actions"][i];
      s += "<tr>";
      s += "<td class='delete-selector' style='display:none'>";
      s += "<input type='checkbox' class='selectcheckbox' ";
      s += "value='" + actionsPkt["actions"].indexOf(actionDef) + "'";
      s += "actionId='" + escapeHTML(actionDef.actionId) + "'></td>";
      s += "<td>" + escapeHTML(actionDef.actionId) + "</td>";

      const serverName = getNickNameFromAction(actionsPkt, actionDef);
      s += "<td>" + escapeHTML(serverName) + "</td>";

      const hostgroupName = getHostgroupNameFromAction(actionsPkt, actionDef);
      s += "<td>" + escapeHTML(hostgroupName) + "</td>";

      const triggerStatus = actionDef.triggerStatus;
      const triggerStatusLabel = "ANY";
      if (triggerStatus !== null)
        triggerStatusLabel = makeTriggerStatusLabel(triggerStatus);
      s += "<td>" + triggerStatusLabel + "</td>";

      const triggerSeverity = actionDef.triggerSeverity;
      const severityLabel = "ANY";
      if (triggerSeverity !== null)
        severityLabel = makeSeverityLabel(triggerSeverity);

      const severityCompType = actionDef.triggerSeverityComparatorType;
      const severityCompLabel = makeSeverityCompTypeLabel(severityCompType);

      s += "<td>" + severityCompLabel + " " + severityLabel + "</td>";

      const command = actionDef.command;
      const incidentTracker = self.incidentTrackersMap[actionDef.command];
      s += "<td>";
      if (incidentTracker)
      {
        s += incidentTracker.nickname;
        switch (incidentTracker.type)
        {
        case hatohol.INCIDENT_TRACKER_REDMINE:
          s += " (" + gettext("Project: ") + incidentTracker.projectId;
          break;
        case hatohol.INCIDENT_TRACKER_HATOHOL:
          s += " (" + gettext("Hatohol");
          break;
        default:
          s += " (" + gettext("Unknown");
        }
        if (incidentTracker.trackerId)
        {
          s += ", " + gettext("Tracker: ") + incidentTracker.trackerId;
        }
        s += ")";
      }
      else
      {
        s += gettext("Unknown");
      }
      s += "</td>";

      s += "<td class='edit-incident-setting-column' style='display:none;'>";
      s += "<input id='edit-incident-setting" + escapeHTML(actionDef.actionId) + "'";
      s += "  type='button' class='btn btn-default'";
      s += "  actionId='" + escapeHTML(actionDef.actionId) + "'";
      s += "  value='" + gettext("EDIT") + "' />";
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

  function setupEditButtons(incidentSettingsData) {
    var i, id, actions = incidentSettingsData["actions"], actionsMap = {};
    var incidentTrackers = self.incidentTrackersData.incidentTrackers;

    for (i = 0; i < actions.length; ++i)
      actionsMap[actions[i].actionId] = actions[i];

    for (i = 0; i < actions.length; ++i) {
      id = "#edit-incident-setting" + actions[i].actionId;
      $(id).click(function() {
        var actionId = this.getAttribute("actionId");
        new HatoholAddActionDialog(load, incidentTrackers, actionsMap[actionId]);
      });
    }

    if (userProfile.hasFlag(hatohol.OPPRVLG_UPDATE_INCIDENT_SETTING)) {
      $(".edit-incident-setting-column").show();
    }
  }

  function onGotIncidentTrackers(incidentTrackersData) {
    self.incidentTrackersData = incidentTrackersData;
    self.incidentTrackersMap = parseIncidentTrackers(self.incidentTrackersData);
    $("#table tbody").empty();
    $("#table tbody").append(drawTableBody(self.incidentSettingsData));
    self.setupCheckboxForDelete($("#delete-incident-setting-button"));
    if (self.userProfile.hasFlag(hatohol.OPPRVLG_DELETE_INCIDENT_SETTING))
      $(".delete-selector").shiftcheckbox();
      $(".delete-selector").show();
    setupEditButtons(self.incidentSettingsData);
  }

  function onGotIncidentSettings(incidentSettingsData) {
    self.incidentSettingsData = incidentSettingsData;
    self.startConnection("incident-tracker", onGotIncidentTrackers);
  }

  function getQuery() {
    var query = {
      type: hatohol.ACTION_INCIDENT_SENDER,
    };
    return 'action?' + $.param(query);
  }

  function load() {
    self.displayUpdateTime();
    self.startConnection(getQuery(), onGotIncidentSettings);
  }
};

IncidentSettingsView.prototype = Object.create(HatoholMonitoringView.prototype);
IncidentSettingsView.prototype.constructor = IncidentSettingsView;
