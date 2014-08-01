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

var IssueSettingsView = function(userProfile) {
  //
  // Variables
  //
  var self = this;
  self.issueSettingsData = null;
  self.issueTrackersData = null;
  self.issueTrackersMap = null;

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

  $("#add-issue-setting-button").click(function() {
    var issueTrackers = self.issueTrackersData.issueTrackers;
    new HatoholAddActionDialog(load, issueTrackers);
  });

  $("#delete-issue-setting-button").click(function() {
    var msg = gettext("Do you delete the selected items ?");
    hatoholNoYesMsgBox(msg, deleteActions);
  });

  $("#edit-issue-trackers-button").click(function() {
    new HatoholIssueTrackersEditor({
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
      s += "<td class='delete-selector'>";
      s += "<input type='checkbox' class='selectcheckbox' " +
        "actionId='" + escapeHTML(actionDef.actionId) + "'></td>";
      s += "<td>" + escapeHTML(actionDef.actionId) + "</td>";

      var serverName = getServerNameFromAction(actionsPkt, actionDef);
      s += "<td>" + escapeHTML(serverName) + "</td>";

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

      var command = actionDef.command;
      var issueTracker = self.issueTrackersMap[actionDef.command];
      s += "<td>";
      if (issueTracker) {
	s += issueTracker.nickname;
        s += " (" + gettext("Project: ") + issueTracker.projectId;
        if (issueTracker.trackerId) {
          s += ", " + gettext("Tracker: ") + issueTracker.trackerId;
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

  function parseIssueTrackers(issueTrackersData) {
    var issueTrackers = issueTrackersData.issueTrackers;
    var i, issueTrackersMap = {};
    for (i = 0; i < issueTrackers.length; i++)
      issueTrackersMap[issueTrackers[i].id] = issueTrackers[i];
    return issueTrackersMap;
  }

  function onGotIssueTrackers(issueTrackersData) {
    self.issueTrackersData = issueTrackersData;
    self.issueTrackersMap = parseIssueTrackers(self.issueTrackersData);
    $("#table tbody").empty();
    $("#table tbody").append(drawTableBody(self.issueSettingsData));
    self.setupCheckboxForDelete($("#delete-issue-setting-button"));
  }

  function onGotIssueSettings(issueSettingsData) {
    self.issueSettingsData = issueSettingsData;
    self.startConnection("issue-trackers", onGotIssueTrackers);
  }

  function getQuery() {
    var query = {
      type: hatohol.ACTION_ISSUE_SENDER,
    };
    return 'action?' + $.param(query);
  };

  function load() {
    self.startConnection(getQuery(), onGotIssueSettings);
  }
};

IssueSettingsView.prototype = Object.create(HatoholMonitoringView.prototype);
IssueSettingsView.prototype.constructor = IssueSettingsView;
