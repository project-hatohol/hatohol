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
  $("#add-action-button").click(function() {
    new HatoholAddActionDialog(load);
  });

  $("#delete-action-button").click(function() {
    var msg = gettext("Delete the selected items ?");
    hatoholNoYesMsgBox(msg, deleteActions);
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
    case hatohol.ACTION_COMMAND:
      return gettext("COMMAND");
    case hatohol.ACTION_RESIDENT:
      return gettext("RESIDENT");
    default:
      return "INVALID: " + type;
    }
  }

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

  function setupEditButtons(actionsPkt)
  {
    var i, id, actions = actionsPkt["actions"], actionsMap = {};

    for (i = 0; i < actions.length; ++i)
      actionsMap[actions[i].actionId] =  actions[i];

    for (i = 0; i < actions.length; ++i) {
      id = "#edit-action" + actions[i].actionId;
      $(id).click(function() {
        var actionId = this.getAttribute("actionId");
        new HatoholAddActionDialog(load, null, actionsMap[actionId]);
      });
    }

    if (userProfile.hasFlag(hatohol.OPPRVLG_UPDATE_ACTION) ||
        userProfile.hasFlag(hatohol.OPPRVLG_UPDATE_ALL_ACTION))
    {
      $(".edit-action-column").show();
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
  function drawTableBody(actionsPkt)
  {
    let s = "";
    for (let actionDef of actionsPkt["actions"])
    {
      s += "<tr>";
      s += "<td class='delete-selector' style='display:none;'>";
      s += "<input type='checkbox' class='selectcheckbox' ";
      s += "value='" + actionsPkt["actions"].indexOf(actionDef) + "'";
      s += "actionId='" + escapeHTML(actionDef.actionId) + "'></td>";
      s += "<td>" + escapeHTML(actionDef.actionId) + "</td>";

      const nickName = getNickNameFromAction(actionsPkt, actionDef);
      s += "<td>" + escapeHTML(nickName) + "</td>";

      const hostName = getHostNameFromAction(actionsPkt, actionDef);
      s += "<td>" + escapeHTML(hostName)   + "</td>";

      const hostgroupName = getHostgroupNameFromAction(actionsPkt, actionDef);
      s += "<td>" + escapeHTML(hostgroupName) + "</td>";

      const triggerBrief = getTriggerBriefFromAction(actionsPkt, actionDef);
      s += "<td>" + escapeHTML(triggerBrief) + "</td>";

      const triggerStatus = actionDef.triggerStatus;
      let triggerStatusLabel = "ANY";
      if (triggerStatus !== null)
        triggerStatusLabel = makeTriggerStatusLabel(triggerStatus);
      s += "<td>" + triggerStatusLabel + "</td>";

      const triggerSeverity = actionDef.triggerSeverity;
      let severityLabel = "ANY";
      if (triggerSeverity !== null)
        severityLabel = makeSeverityLabel(triggerSeverity);

      const severityCompType = actionDef.triggerSeverityComparatorType;
      let severityCompLabel = "";
      if (triggerSeverity !== null)
        severityCompLabel = makeSeverityCompTypeLabel(severityCompType);

      s += "<td>" + severityCompLabel + " " + severityLabel + "</td>";

      const type = actionDef.type;
      const typeLabel = makeTypeLabel(type);
      s += "<td>" + typeLabel + "</td>";

      let workingDir = actionDef.workingDirectory;
      if (!workingDir)
        workingDir = "N/A";
      s += "<td>" + escapeHTML(workingDir) + "</td>";

      const command = actionDef.command;
      s += "<td>" + escapeHTML(command) + "</td>";

      const timeout = actionDef.timeout;
      let timeoutLabel;
      if (timeout === 0)
        timeoutLabel = gettext("No limit");
      else
        timeoutLabel = timeout / 1000;
      s += "<td>" + escapeHTML(timeoutLabel) + "</td>";

      s += "<td class='edit-action-column' style='display:none;'>";
      s += "<input id='edit-action" + escapeHTML(actionDef.actionId) + "'";
      s += "  type='button' class='btn btn-default'";
      s += "  actionId='" + escapeHTML(actionDef.actionId) + "'";
      s += "  value='" + gettext("EDIT") + "' />";
      s += "</td>";
      s += "</tr>";
    }

    return s;
  }

  function updateCore(rawData) {
    $("#table tbody").empty();
    $("#table tbody").append(drawTableBody(rawData));
    self.setupCheckboxForDelete($("#delete-action-button"));
    if (self.userProfile.hasFlag(hatohol.OPPRVLG_DELETE_ACTION) ||
    self.userProfile.hasFlag(hatohol.OPPRVLG_DELETE_ALL_ACTION)) {
      $(".delete-selector").shiftcheckbox();
      $(".delete-selector").show();
    }
    setupEditButtons(rawData);
    self.displayUpdateTime();
  }

  function load() {
    self.startConnection('action', updateCore);
  }
};

ActionsView.prototype = Object.create(HatoholMonitoringView.prototype);
ActionsView.prototype.constructor = ActionsView;
