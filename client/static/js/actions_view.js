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

var ActionsView = function() {
  //
  // Variables
  //
  var numSelected = 0;

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
    new HatoholAddActionDialog(addSucceededCb);
  });

  $("#delete-action-button").click(function() {
    var msg = gettext("Do you delete the selected items ?");
    hatoholNoYesMsgBox(msg, deleteActions);
  });

  function addSucceededCb() {
    startConnection('action', updateCore);
  }

  //
  // Event handlers for forms in the main view
  //
  function setupCheckboxHandler() {
    $(".selectcheckbox").change(function() {
      var check = $(this).is(":checked");
      var prevNumSelected = numSelected;
      if (check)
        numSelected += 1;
      else
        numSelected -= 1;
      if (prevNumSelected == 0 && numSelected == 1)
        $("#delete-action-button").attr("disabled", false);
      else if (prevNumSelected == 1 && numSelected == 0)
        $("#delete-action-button").attr("disabled", true);
    });
  }

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
      msg = "Failed:\n" + data.message
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
    var checkbox = $(".selectcheckbox");
    var deletedIdArray = {count:0, total:0, errors:0};
    for (var i = 0; i < checkbox.length; i++) {
      if (!checkbox[i].checked)
        continue;
      var actionId = checkbox[i].getAttribute("actionId");
      deletedIdArray[actionId] = true;
      deletedIdArray.count++;
      deletedIdArray.total++;
      deleteOneAction(actionId, deletedIdArray);
    }
    hatoholInfoMsgBox(gettext("Deleting..."));
  }

  function deleteOneAction(id, deletedIdArray) {
    new HatoholConnector({
      url: '/action/' + id,
      request: "DELETE",
      context: deletedIdArray,
      replyCallback: function(data, parser, context) {
        parseDeleteActionResult(data, context);
      },
      connectErrorCallback: function(XMLHttpRequest,
                                     textStatus, errorThrown) {
        var errorMsg = "Error: " + XMLHttpRequest.status + ": " +
          XMLHttpRequest.statusText;
        hatoholErrorMsgBox(errorMsg);
        deletedIdArray.errors++;
      },
      completionCallback: function(context) {
        compleOneDelAction(context);
      },
    });
  }

  function parseDeleteActionResult(data, deletedIdArray) {
    if (!parseResult(data))
      return;
    if (!(data.id in deletedIdArray)) {
      alert("Fatal Error: You should reload page.\nID: " + data.id +
            " is not in deletedIdArray: " + deletedIdArray);
      deletedIdArray.errors++;
      return;
    }
    delete deletedIdArray[data.id];
  }

  function compleOneDelAction(deletedIdArray) {
    deletedIdArray.count--;
    var completed = deletedIdArray.total - deletedIdArray.count;
    hatoholErrorMsgBox(gettext("Deleting...") + " " + completed +
                       " / " + deletedIdArray.total);
    if (deletedIdArray.count > 0)
      return;

    // close dialogs
    hatoholInfoMsgBox(gettext("Completed. (Number of errors: ") +
                      deletedIdArray.errors + ")");

    // update the main view
    startConnection('action', updateCore);
  }

  //
  // Functions for make the main view.
  //
  function makeNamelessServerLabel(serverId) {
    return "(ID:" + serverId + ")";
  }

  function makeNamelessHostLabel(serverId, hostId) {
    return "(S" + serverId + "-H" + hostId + ")";
  }

  function makeNamelessTriggerLabel(triggerId) {
    return "(T" + triggerId + ")";
  }

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
  function getServerName(actionsPkt, actionDef) {
    var serverId = actionDef["serverId"];
    if (!serverId)
      return null;
      var server = actionsPkt["servers"][serverId];
    if (!server)
      return makeNamelessServerLabel(serverId);
    var serverName = actionsPkt["servers"][serverId]["name"];
    if (!serverName)
      return makeNamelessServerLabel(serverId);
    return serverName;
  }

  function getHostName(actionsPkt, actionDef) {
    var hostId = actionDef["hostId"]
    if (!hostId)
      return null;
    var serverId = actionDef["serverId"];
    if (!serverId)
      return makeNamelessHostLabel(serverId, hostId);
      var server = actionsPkt["servers"][serverId];
    if (!server)
      return null;
    var hostArray = server["hosts"]
    if (!hostArray)
      return makeNamelessHostLabel(serverId, hostId);
    var host = hostArray[hostId];
    if (!host)
      return makeNamelessHostLabel(serverId, hostId);
    var hostName = host["name"];
      if (!hostName)
        return makeNamelessHostLabel(serverId, hostId);
    return hostName;
  }

  function getTriggerBrief(actionsPkt, actionDef) {
    var triggerId = actionDef["triggerId"]
    if (!triggerId)
      return null;
    var serverId = actionDef["serverId"];
    if (!serverId)
      return makeNamelessTriggerLabel(triggerId);
    var server = actionsPkt["servers"][serverId];
      if (!server)
        return null;
    var triggerArray = server["triggers"]
    if (!triggerArray)
      return makeNamelessTriggerLabel(triggerId);
    var trigger = triggerArray[triggerId];
    if (!trigger)
        return makeNamelessTriggerLabel(triggerId);
    var triggerBrief = trigger["brief"];
    if (!triggerBrief)
        return makeNamelessTriggerLabel(triggerId);
    return triggerBrief;
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
      s += "<tr>"
      s += "<td><input type='checkbox' class='selectcheckbox' " +
        "actionId='" + escapeHTML(actionDef.actionId) + "'></td>";
      s += "<td>" + escapeHTML(actionDef.actionId) + "</td>";

      var serverName = getServerName(actionsPkt, actionDef);
      if (!serverName)
        serverName = "ANY";
      s += "<td>" + escapeHTML(serverName) + "</td>";

      var hostName = getHostName(actionsPkt, actionDef);
      if (!hostName)
        hostName = "ANY";
      s += "<td>" + escapeHTML(hostName)   + "</td>";

      /* Not supported
      var hostGroupName = "unsupported";
      s += "<td>" + escapeHTML(hostGroupName) + "</td>";
      */

      var triggerBrief = getTriggerBrief(actionsPkt, actionDef);
      if (!triggerBrief)
        triggerBrief = "ANY";
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
    setupCheckboxHandler();
    numSelected = 0;
  }

  //
  // main code
  //
  startConnection('action', updateCore);
};
