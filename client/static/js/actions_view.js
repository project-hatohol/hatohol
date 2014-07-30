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

var ActionsView = function(userProfile) {
  //
  // Variables
  //
  var self = this;

  // call the constructor of the super class
  HatoholMonitoringView.apply(this, [userProfile]);

  //
  // main code
  //
  if (userProfile.hasFlag(hatohol.OPPRVLG_CREATE_ACTION))
    $("#add-action-button").show();
  if (userProfile.hasFlag(hatohol.OPPRVLG_DELETE_ACTION) ||
      userProfile.hasFlag(hatohol.OPPRVLG_DELETE_ALL_ACTION)) {
    $("#delete-action-button").show();
  }
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

  $("#add-action-button").click(function() {
    new HatoholAddActionDialog(load);
  });

  $("#delete-action-button").click(function() {
    var msg = gettext("Do you delete the selected items ?");
    hatoholNoYesMsgBox(msg, deleteActions);
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

  //
  // delete-action dialog
  //
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
  function makeTypeLabel(type) {
    switch(type) {
    case ACTION_COMMAND:
      return gettext("COMMAND");
    case ACTION_RESIDENT:
      return gettext("RESIDENT");
    default:
      return "INVALID: " + type;
    }
  }

  function makeSeverityCompTypeLabel(compType) {
    switch(compType) {
    case CMP_EQ:
      return "=";
    case CMP_EQ_GT:
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

  function getHostNameFromAction(actionsPkt, actionDef) {
    var serverId = actionDef["serverId"];
    var hostId = actionDef["hostId"];
    if (!hostId)
      return "ANY";
    return getHostName(actionsPkt["servers"][serverId], hostId);
  }

  function getTriggerBriefFromAction(actionsPkt, actionDef) {
    var serverId = actionDef["serverId"];
    var triggerId = actionDef["triggerId"];
    if (!triggerId)
      return "ANY";
    return getTriggerBrief(actionsPkt["servers"][serverId], triggerId);
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
      s += "<td class='delete-selector' style='display:none;'>";
      s += "<input type='checkbox' class='selectcheckbox' " +
        "actionId='" + escapeHTML(actionDef.actionId) + "'></td>";
      s += "<td>" + escapeHTML(actionDef.actionId) + "</td>";

      var serverName = getServerNameFromAction(actionsPkt, actionDef);
      s += "<td>" + escapeHTML(serverName) + "</td>";

      var hostName = getHostNameFromAction(actionsPkt, actionDef);
      s += "<td>" + escapeHTML(hostName)   + "</td>";

      var hostgroupName = getHostgroupNameFromAction(actionsPkt, actionDef);
      s += "<td>" + escapeHTML(hostgroupName) + "</td>";

      var triggerBrief = getTriggerBriefFromAction(actionsPkt, actionDef);
      s += "<td>" + escapeHTML(triggerBrief) + "</td>";

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

      var type = actionDef.type;
      var typeLabel = makeTypeLabel(type);
      s += "<td>" + typeLabel + "</td>";

      var workingDir = actionDef.workingDirectory;
      if (!workingDir)
        workingDir = "N/A";
      s += "<td>" + escapeHTML(workingDir) + "</td>";

      var command = actionDef.command;
      s += "<td>" + escapeHTML(command) + "</td>";

      var timeout = actionDef.timeout;
      if (timeout == 0)
        timeout = gettext("No limit");
      s += "<td>" + escapeHTML(timeout) + "</td>";

      s += "</tr>";
    }

    return s;
  }

  function updateCore(rawData) {
    $("#table tbody").empty();
    $("#table tbody").append(drawTableBody(rawData));
    self.setupCheckboxForDelete($("#delete-action-button"));
  }

  function load() {
    self.startConnection('action', updateCore);
  }
};

ActionsView.prototype = Object.create(HatoholMonitoringView.prototype);
ActionsView.prototype.constructor = ActionsView;
