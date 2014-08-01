/*
 * Copyright (C) 2013 Project Hatohol
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

var HatoholAddActionDialog = function(changedCallback, issueTrackers) {
  var self = this;

  var IDX_SELECTED_SERVER  = 0;
  var IDX_SELECTED_HOST_GROUP = 1
  var IDX_SELECTED_HOST    = 2;
  var IDX_SELECTED_TRIGGER = 3;
  self.selectedId = new Array();
  self.selectedId[IDX_SELECTED_SERVER]  = null;
  self.selectedId[IDX_SELECTED_HOST_GROUP] = null;
  self.selectedId[IDX_SELECTED_HOST]    = null;
  self.selectedId[IDX_SELECTED_TRIGGER] = null;

  self.changedCallback = changedCallback;
  self.issueTrackers = issueTrackers;
  self.forIssueSetting = !!issueTrackers;

  self.windowTitle = self.forIssueSetting ?
    gettext("ADD ISSUE TRACKING SETTING") : gettext("ADD ACTION");

  var dialogButtons = [{
    text: gettext("ADD"),
    click: addButtonClickedCb,
  }, {
    text: gettext("CANCEL"),
    click: cancelButtonClickedCb,
  }];
  var dialogAttrs = { width: "auto" };

  // call the constructor of the super class
  HatoholDialog.apply(
    this, ["add-action-dialog", self.windowTitle,
           dialogButtons, dialogAttrs]);

  setTimeout(function() {
    self.setAddButtonState(!!self.getCommand());
  }, 1);

  //
  // Dialog button handlers
  //
  function addButtonClickedCb() {
    if (validateAddParameters()) {
      makeQueryData();
      hatoholInfoMsgBox(gettext("Now creating an action ..."));
      postAddAction();
    }
  }

  function cancelButtonClickedCb() {
    self.closeDialog();
  }

  //
  // Event handlers for forms in the add-action dialog
  //
  $("#selectTriggerSeverity").change(function() {
    var severity = $(this).val();
    if (severity != "ANY")
      $("#selectTriggerSeverityCompType").css("visibility","visible");
    else
      $("#selectTriggerSeverityCompType").css("visibility","hidden");
  })

  $("#selectServerId").change(function() {
    var val = $(this).val();
    if (val == "SELECT")
      new HatoholServerSelector(serverSelectedCb);
    else
      setSelectedServerId(val);
  })

  $("#selectHostgroupId").change(function() {
    var val = $(this).val();
    if (val == "SELECT") {
      new HatoholHostgroupSelector(self.selectedId[IDX_SELECTED_SERVER],
                                   hostgroupSelectedCb);
    } else {
      setSelectedHostgroupId(val);
    }
  })

  $("#selectHostId").change(function() {
    var val = $(this).val();
    if (val == "SELECT") {
      new HatoholHostSelector(self.selectedId[IDX_SELECTED_SERVER],
                              self.selectedId[IDX_SELECTED_HOST_GROUP],
                              hostSelectedCb);
    } else {
      setSelectedHostId(val);
    }
  })

  $("#selectTriggerId").change(function() {
    var val = $(this).val();
    if (val == "SELECT") {
      new HatoholTriggerSelector(self.selectedId[IDX_SELECTED_SERVER],
                                 self.selectedId[IDX_SELECTED_HOST],
                                 triggerSelectedCb);
    } else {
      setSelectedTriggerId(val);
    }
  })

  function serverSelectedCb(serverInfo) {
    var label = "";
    if (serverInfo)
      label = serverInfo.id + ": " + serverInfo.hostName;
    selectedCallback($("#selectServerId"), serverInfo,
                     IDX_SELECTED_SERVER, label,
                     fixupSelectHostgroupAndHostBox);
  }

  function hostgroupSelectedCb(hostgroupInfo) {
    var label = "";
    if (hostgroupInfo)
      label = hostgroupInfo.groupId + ": " + hostgroupInfo.groupName;
    selectedCallback($("#selectHostgroupId"), hostgroupInfo,
                     IDX_SELECTED_HOST_GROUP, label, fixupSelectHostBox,
                     "groupId");
  }

  function hostSelectedCb(hostInfo) {
    var label = "";
    if (hostInfo)
      label = hostInfo.hostName;
    selectedCallback($("#selectHostId"), hostInfo, IDX_SELECTED_HOST,
                     label, fixupSelectTriggerBox);
  }

  function triggerSelectedCb(triggerInfo) {
    var label = "";
    if (triggerInfo)
      label = triggerInfo.brief;
    selectedCallback($("#selectTriggerId"), triggerInfo, IDX_SELECTED_TRIGGER,
                     label, null);
  }

  function selectedCallback(jQObjSelectId, response, selectedIdIndex,
                            label, fixupSelectBoxFunc, idName) {
    var numOptions = jQObjSelectId.children().length;
    var currSelectedId = self.selectedId[selectedIdIndex];
    console.log(response);
    if (!response) {
      if (!currSelectedId)
        jQObjSelectId.val("ANY");
      else
        jQObjSelectId.val(currSelectedId);
      return;
    }

    var responseId;
    if (idName)
      responseId = response[idName];
    else
      responseId = response.id;
    if (numOptions == 3) {
      if (responseId == currSelectedId)
        return;
      jQObjSelectId.children('option:last-child').remove();
    }
    setSelectedId(selectedIdIndex, responseId, fixupSelectBoxFunc);
    jQObjSelectId.append($("<option>").text(label).val(responseId));
    jQObjSelectId.val(responseId);
  }

  function setSelectedId(selectedIdIndex, value, fixupSelectBoxFunc) {
    if (value == "ANY")
      self.selectedId[selectedIdIndex] = null;
    else
      self.selectedId[selectedIdIndex] = value;
    if (fixupSelectBoxFunc)
      fixupSelectBoxFunc(value);
  }

  function fixupSelectBox(jQObjSelect, newValue, selectedIdSetter) {
    var numOptions = jQObjSelect.children().length;
    if (newValue == "ANY") {
      for (var i = numOptions; i > 1; i--)
        jQObjSelect.children('option:last-child').remove();
      jQObjSelect.val("ANY");
      selectedIdSetter("ANY");
      return;
    }
    if (numOptions == 1) {
      var label = "== " + gettext("SELECT") + " ==";
      jQObjSelect.append($("<option>").html(label).val("SELECT"));
    }
    if (numOptions == 3)
      jQObjSelect.children('option:last-child').remove();
    jQObjSelect.val("ANY");
    selectedIdSetter("ANY");
  }

  function setSelectedServerId(value) {
    setSelectedId(IDX_SELECTED_SERVER, value, fixupSelectHostgroupAndHostBox);
  }

  function fixupSelectHostgroupAndHostBox(newServerId) {
    fixupSelectHostgroupBox(newServerId);
    fixupSelectHostBox(newServerId);
  }

  function fixupSelectHostgroupBox(newServerId) {
    fixupSelectBox($("#selectHostgroupId"), newServerId, setSelectedHostgroupId);
  }

  function setSelectedHostgroupId(value) {
    setSelectedId(IDX_SELECTED_HOST_GROUP, value, fixupSelectHostBox);
  }

  function fixupSelectHostBox(newServerId) {
    fixupSelectBox($("#selectHostId"), newServerId, setSelectedHostId);
  }

  function setSelectedHostId(value) {
    setSelectedId(IDX_SELECTED_HOST, value, fixupSelectTriggerBox);
  }

  function fixupSelectTriggerBox(newHostId) {
    fixupSelectBox($("#selectTriggerId"), newHostId, setSelectedTriggerId);
  }

  function setSelectedTriggerId(value) {
    setSelectedId(IDX_SELECTED_TRIGGER, value, null);
  }

  //
  // General class methods
  //
  function getCommandType() {
    if (self.forIssueSetting)
      return hatohol.ACTION_ISSUE_SENDER;

    var type = $("#selectType").val();
    switch(type) {
    case "ACTION_COMMAND":
      return hatohol.ACTION_COMMAND;
    case "ACTION_RESIDENT":
      return hatohol.ACTION_RESIDENT;
    default:
      alert("Unknown command type: " + type);
    }
    return undefined;
  }

  function getStatusValue() {
    var status = $("#selectTriggerStatus").val();
    switch(status) {
    case "ANY":
      return undefined;
    case "TRIGGER_STATUS_OK":
      return TRIGGER_STATUS_OK;
    case "TRIGGER_STATUS_PROBLEM":
      return TRIGGER_STATUS_PROBLEM;
    default:
      alert("Unknown status: " + status);
    }
    return undefined;
  }

  function getSeverityValue() {
    var severity = $("#selectTriggerSeverity").val();
    switch(severity) {
    case "ANY":
      return undefined;
    case "INFO":
      return TRIGGER_SEVERITY_INFO;
    case "WARNING":
      return TRIGGER_SEVERITY_WARNING;
    case "ERROR":
      return TRIGGER_SEVERITY_ERROR;
    case "CRITICAL":
      return TRIGGER_SEVERITY_CRITICAL;
    case "EMERGENCY":
      return TRIGGER_SEVERITY_EMERGENCY;
    default:
      alert("Unknown severity: " + severity);
    }
    return undefined;
  }

  function getSeverityCompTypeValue() {
    var compType = $("#selectTriggerSeverityCompType").val();
    switch(compType) {
    case "CMP_EQ":
      return hatohol.CMP_EQ;
    case "CMP_EQ_GT":
      return hatohol.CMP_EQ_GT;
    default:
      alert("Unknown severity: " + severity);
    }
    return undefined;
  }

  function makeQueryData() {
      var queryData = {};
      var serverId = $("#selectServerId").val();
      if (serverId != "ANY")
        queryData.serverId = serverId;
      var hostgroupId = $("#selectHostgroupId").val();
      if (hostgroupId != "ANY")
        queryData.hostgroupId = hostgroupId;
      var hostId = $("#selectHostId").val();
      if (hostId != "ANY")
        queryData.hostId = hostId;
      var triggerId = $("#selectTriggerId").val();
      if (triggerId != "ANY")
        queryData.triggerId = triggerId;
      queryData.type = getCommandType();
      queryData.timeout = $("#inputTimeout").val();
      queryData.command = self.getCommand();
      queryData.workingDirectory = $("#inputWorkingDir").val();
      queryData.triggerStatus   = getStatusValue();
      queryData.triggerSeverity = getSeverityValue();
      queryData.triggerSeverityCompType = getSeverityCompTypeValue();
      return queryData;
  }

  function postAddAction() {
    new HatoholConnector({
      url: "/action",
      request: "POST",
      data: makeQueryData(),
      replyCallback: replyCallback,
      parseErrorCallback: hatoholErrorMsgBoxForParser,
    });
  }

  function replyCallback(reply, parser) {
    self.closeDialog();
    hatoholInfoMsgBox(gettext("Successfully created."));

    if (self.changedCallback)
      self.changedCallback();
  }

  function validateAddParameters() {
    if (self.getCommand() == "") {
      hatoholErrorMsgBox(gettext("Command is empty!"));
      return false;
    }
    return true;
  }
}

HatoholAddActionDialog.prototype = Object.create(HatoholDialog.prototype);
HatoholAddActionDialog.prototype.constructor = HatoholAddActionDialog;

HatoholAddActionDialog.prototype.createMainElement = function() {
  var self = this;
  var div = $(makeMainDivHTML());
  return div;

  function makeTriggerConditionArea() {
    var s = "";
    s += '<h3>' + gettext("Condition") + '</h3>'
    s += '<form class="form-inline">'
    s += '  <label>' + gettext("Server") + '</label>'
    s += '  <select id="selectServerId">'
    s += '    <option value="ANY">ANY</option>'
    s += '    <option value="SELECT">== ' + gettext("SELECT") + ' ==</option>'
    s += '  </select>'

    s += '  <label>' + gettext("Hostgroup") + '</label>'
    s += '  <select id="selectHostgroupId">'
    s += '    <option value="ANY">ANY</option>'
    s += '  </select>'

    if (!self.forIssueSetting) {
      s += '  <label>' + gettext("Host") + '</label>'
      s += '  <select id="selectHostId">'
      s += '    <option value="ANY">ANY</option>'
      s += '  </select>'
    }

    s += '  <label>' + gettext("Trigger") + '</label>'
    s += '  <select id="selectTriggerId">'
    s += '    <option value="ANY">ANY</option>'
    s += '  </select>'
    s += '</form>'

    s += '<form class="form-inline">'
    s += '  <label>' + gettext("Status") + '</label>'
    s += '  <select id="selectTriggerStatus">'
    s += '    <option value="ANY">ANY</option>'
    s += '    <option value="TRIGGER_STATUS_OK">' + gettext("OK") + '</option>'
    s += '    <option value="TRIGGER_STATUS_PROBLEM">' + gettext("Problem") + '</option>'
    s += '  </select>'

    s += '  <label>' + gettext("Severity") + '</label>'
    s += '  <select id="selectTriggerSeverity">'
    s += '    <option value="ANY">ANY</option>'
    s += '    <option value="INFO">' + gettext("Information") + '</option>'
    s += '    <option value="WARNING">' + gettext("Warning") + '</option>'
    s += '    <option value="ERROR">' + gettext("Average") + '</option>'
    s += '    <option value="CRITICAL">' + gettext("High") + '</option>'
    s += '    <option value="EMERGENCY">' + gettext("Disaster") + '</option>'
    s += '  </select>'
    s += '  <select id="selectTriggerSeverityCompType" style="visibility:hidden;">'
    s += '    <option value="CMP_EQ">' + gettext("Equal to") + '</option>'
    s += '    <option value="CMP_EQ_GT">' + gettext("Equal to or greater than") + '</option>'
    s += '  </select>'
    s += '</form>'
    return s;
  }

  function makeExecutionParameterArea() {
    var s = "";
    s += '<h3>' + gettext("Execution parameters") + '</h3>'
    s += '<form class="form-inline">'
    s += '  <label>' + gettext("Templates ") + '</label>'
    s += '  <input id="actor-mail-dialog-button" type="button" value=' + gettext("e-mail") + ' />'
    s += '</form>'

    s += '<form class="form-inline">'
    s += '  <label>' + gettext("Type") + '</label>'
    s += '  <select id="selectType">'
    s += '    <option value="ACTION_COMMAND">' + gettext("COMMAND") + '</option>'
    s += '    <option value="ACTION_RESIDENT">' + gettext("RESIDENT") + '</option>'
    s += '  </select>'

    s += '  <label for="inputTimeout">' + gettext("Time-out (sec)") + '</label>'
    s += '  <input id="inputTimeout" type="text" value="0">'
    s += '  <label for="inputTimeout">(0: ' + gettext("No limit") + ') </label>'
    s += '</form>'

    s += '<form class="form-inline">'
    s += '  <label for="inputActionCommand">' + gettext("Command parameter") + '</label>'
    s += '  <input id="inputActionCommand" type="text" style="width:100%;" value="">'
    s += '</form>'

    s += '<form class="form-inline">'
    s += '  <label for="inputWorkingDir">' + gettext("Execution directory") + '</label>'
    s += '  <input id="inputWorkingDir" type="text" value="" style="width:100%;">'
    s += '</form>'
    return s;
  }

  function makeIssueTrackerArea() {
    var s = "", i, issueTracker;
    s += '<h3>' + gettext("Issue Tracking Server") + '</h3>';
    s += '<form class="form-inline">';
    s += '<select id="selectIssueTracker">';
    s += '</select>';
    s += '<input id="editIssueTrackers" type="button" '
    s += '       style="margin-left: 2px;" ';
    s +='        value="' + gettext('EDIT') + '" />';
    s += '</form>'
    return s;
  }

  function makeMainDivHTML() {
    var s = "";
    s += '<div id="add-action-div">'
    s += makeTriggerConditionArea();
    if (self.forIssueSetting)
      s += makeIssueTrackerArea();
    else
      s += makeExecutionParameterArea();
    s += '</div>'
    return s;
  }
}

HatoholAddActionDialog.prototype.getCommand = function() {
  if (this.forIssueSetting)
    return $("#selectIssueTracker").val();
  else
    return $("#inputActionCommand").val();
}

HatoholAddActionDialog.prototype.updateIssueTrackers = function(issueTrackers) {
  var label, issueTraker;

  if (!this.forIssueSetting)
    return;

  $("#selectIssueTracker").empty();
  for (i = 0; i < issueTrackers.length; i++) {
    issueTracker = issueTrackers[i];
    label = "" + issueTracker.id + ": " + issueTracker.nickname;
    label += " (" + gettext("Project: ")  + issueTracker.projectId;
    if (issueTracker.trackerId)
      label += ", " + gettext("Tracker: ") + issueTracker.trackerId;
    label += ")";
    $("#selectIssueTracker").append(
      $("<option>", {
        value: issueTracker.id,
        text: label,
      })
    );
  }
}

HatoholAddActionDialog.prototype.setupIssueTrackersEditor = function()
{
  var self = this;
  var changedCallback = function(issueTrackers) {
    self.issueTrackers = issueTrackers;
    self.updateIssueTrackers(issueTrackers);
    if (self.changedCallback)
      self.changedCallback();
    self.setAddButtonState(!!self.getCommand());
  }
  $("#editIssueTrackers").click(function() {
    new HatoholIssueTrackersEditor({
      changedCallback: changedCallback,
    });
  });
  $("#selectIssueTracker").change(function() {
    self.setAddButtonState(!!self.getCommand());
  });
  changedCallback(self.issueTrackers);
}

HatoholAddActionDialog.prototype.onAppendMainElement = function() {
  var self = this;

  $("#actor-mail-dialog-button").click(function() {
    var currCommand = $("#inputActionCommand").val();
    new HatoholActorMailDialog(applyCallback, currCommand);
  });

  $("#inputActionCommand").keyup(function() {
    fixupAddButtonState();
  });

  function applyCallback(type, commandDesc) {
    switch (type) {
    case hatohol.ACTION_COMMAND:
    case hatohol.ACTION_RESIDENT:
      $("#selectType").val(type);
      break;
    default:
      var msg = gettext("The template script returned the invalid value.") + " (command type: " + type + ")";
      hatoholErrorMsgBox(gettext(msg));
      return;
    }
    $("#inputActionCommand").val(commandDesc);
    fixupAddButtonState();
  }

  function fixupAddButtonState() {
    if ($("#inputActionCommand").val())
      self.setAddButtonState(true);
    else
      self.setAddButtonState(false);
  }

  if (self.forIssueSetting)
    self.setupIssueTrackersEditor();
}

HatoholAddActionDialog.prototype.setAddButtonState = function(state) {
  var btn = $(".ui-dialog-buttonpane").find("button:contains(" +
              gettext("ADD") + ")");
  if (state) {
     btn.removeAttr("disabled");
     btn.removeClass("ui-state-disabled");
  } else {
     btn.attr("disabled", "disabled");
     btn.addClass("ui-state-disabled");
  }
}
