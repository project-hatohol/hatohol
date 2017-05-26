/*
 * Copyright (C) 2013 Project Hatohol
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

// TODO: use hash arguments
var HatoholAddActionDialog = function(changedCallback, incidentTrackers, actionDef) {
  var self = this;

  var IDX_SELECTED_SERVER  = 0;
  var IDX_SELECTED_HOST_GROUP = 1;
  var IDX_SELECTED_HOST    = 2;
  var IDX_SELECTED_TRIGGER = 3;
  self.selectedId = [];
  self.selectedId[IDX_SELECTED_SERVER]  = actionDef ? actionDef.serverId : null;
  self.selectedId[IDX_SELECTED_HOST_GROUP] = actionDef ? actionDef.hostgroupId : null;
  self.selectedId[IDX_SELECTED_HOST]    = actionDef ? actionDef.hostId : null;
  self.selectedId[IDX_SELECTED_TRIGGER] = actionDef ? actionDef.triggerId : null;
  self.actionDef = actionDef ? actionDef : null;
  self.applyButtonTitle = actionDef ? gettext("APPLY") : gettext("ADD");
  self.targetId = actionDef ? actionDef.actionId : null;

  self.changedCallback = changedCallback;
  self.incidentTrackers = incidentTrackers;
  self.forIncidentSetting = !!incidentTrackers;

  if (self.forIncidentSetting) {
    self.windowTitle = self.targetId ?
    gettext("EDIT INCIDENT TRACKING SETTING") : gettext("ADD INCIDENT TRACKING SETTING");
  } else {
    self.windowTitle = self.targetId ?
    gettext("EDIT ACTION") : gettext("ADD ACTION");
  }

  var dialogButtons = [{
    text: self.actionDef ? gettext("APPLY") : gettext("ADD"),
    click: applyButtonClickedCb,
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
    self.setApplyButtonState(!!self.getCommand());
  }, 1);

  //
  // Dialog button handlers
  //
  function applyButtonClickedCb() {
    if (validateAddParameters()) {
      makeQueryData();
      if (self.actionDef) {
        hatoholInfoMsgBox(gettext("Now updating an action ..."));
      } else {
        hatoholInfoMsgBox(gettext("Now creating an action ..."));
      }
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
  });

  $("#selectServerId").change(function() {
    var val = $(this).val();
    if (val == "SELECT")
      new HatoholServerSelector(serverSelectedCb);
    else
      setSelectedServerId(val);
  });

  $("#selectHostgroupId").change(function() {
    var val = $(this).val();
    if (val == "SELECT") {
      new HatoholHostgroupSelector(self.selectedId[IDX_SELECTED_SERVER],
                                   hostgroupSelectedCb);
    } else {
      setSelectedHostgroupId(val);
    }
  });

  $("#selectHostId").change(function() {
    var val = $(this).val();
    if (val == "SELECT") {
      new HatoholHostSelector(self.selectedId[IDX_SELECTED_SERVER],
                              self.selectedId[IDX_SELECTED_HOST_GROUP],
                              hostSelectedCb);
    } else {
      setSelectedHostId(val);
    }
  });

  $("#selectTriggerId").change(function() {
    var val = $(this).val();
    if (val == "SELECT") {
      new HatoholTriggerSelector(self.selectedId[IDX_SELECTED_SERVER],
                                 self.selectedId[IDX_SELECTED_HOST],
                                 triggerSelectedCb);
    } else {
      setSelectedTriggerId(val);
    }
  });

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

  function fixupSelectBox(jQObjSelect, newValue, selectedIdSetter, ignoreAnySelected) {
    var numOptions = jQObjSelect.children().length;
    if (newValue == "ANY" && !ignoreAnySelected) {
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
    setSelectedId(IDX_SELECTED_HOST_GROUP, value, function (newServerId) {
      fixupSelectBox($("#selectHostId"), newServerId, setSelectedHostId, true);
    });
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
    if (self.forIncidentSetting)
      return hatohol.ACTION_INCIDENT_SENDER;

    var type = $("#selectType").val();
    switch(type) {
    case "ACTION_COMMAND":
      return hatohol.ACTION_COMMAND;
    case "ACTION_RESIDENT":
      return hatohol.ACTION_RESIDENT;
    default:
      hatoholErrorMsgBox(gettext("Unknown command type: ") + type);
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
      hatoholErrorMsgBox(gettext("Unknown status: ") + status);
    }
    return undefined;
  }

  function getSeverityValue() {
    var severity = $("#selectTriggerSeverity").val();
    switch(severity) {
    case "ANY":
      return undefined;
    case "UNKNOWN":
      return TRIGGER_SEVERITY_UNKNOWN;
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
      hatoholErrorMsgBox(gettext("Unknown severity: ") + severity);
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
      hatoholErrorMsgBox(gettext("Unknown severity compare type: ") + compType);
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
      if (!isNaN($("#inputTimeout").val()))
        queryData.timeout = $("#inputTimeout").val() * 1000; // in MSec.
      queryData.command = self.getCommand();
      queryData.workingDirectory = $("#inputWorkingDir").val();
      queryData.triggerStatus   = getStatusValue();
      queryData.triggerSeverity = getSeverityValue();
      queryData.triggerSeverityCompType = getSeverityCompTypeValue();
      return queryData;
  }

  function postAddAction() {
    var url = "/action";
    if (self.targetId)
       url += "/" + self.targetId;
    new HatoholConnector({
      url: url,
      request: self.targetId ? "PUT" : "POST",
      data: makeQueryData(),
      replyCallback: replyCallback,
      parseErrorCallback: hatoholErrorMsgBoxForParser,
    });
  }

  function replyCallback(reply, parser) {
    self.closeDialog();
    if (self.actionDef) {
      hatoholInfoMsgBox(gettext("Successfully updated."));
    } else {
      hatoholInfoMsgBox(gettext("Successfully created."));
    }
    if (self.changedCallback)
      self.changedCallback();
  }

  function validateAddParameters() {
    if (self.getCommand() === "") {
      hatoholErrorMsgBox(gettext("Command is empty!"));
      return false;
    }
    return true;
  }

  function setupTimeOutValue(timeout) {
    $("#inputTimeout").val(timeout / 1000);
  }

  function setupActionCommand(command) {
    $("#inputActionCommand").val(command);
  }

  function setupWorkingDirectory(directory) {
     $("#inputWorkingDir").val(directory);
  }

  function setupIncidentTracker(tracker) {
    $("#selectIncidentTracker").val(tracker);
  }

  function setupCommandType(type) {
    var typeSelector = $("#selectType");
    switch(type) {
    case hatohol.ACTION_COMMAND:
      typeSelector.val("ACTION_COMMAND");
      break;
    case hatohol.ACTION_RESIDENT:
      typeSelector.val("ACTION_RESIDENT");
      break;
    case hatohol.ACTION_INCIDENT_SENDER:
      break;
    default:
      hatoholErrorMsgBox(gettext("Unknown command type: ") + type);
      break;
    }
  }

  function setupTriggerStatusValue(status) {
    var statusSelector = $("#selectTriggerStatus");
    switch(status) {
    case null:
      statusSelector.val("ANY");
      break;
    case hatohol.TRIGGER_STATUS_OK:
      statusSelector.val("TRIGGER_STATUS_OK");
      break;
    case hatohol.TRIGGER_STATUS_PROBLEM:
      statusSelector.val("TRIGGER_STATUS_PROBLEM");
      break;
    default:
      hatoholErrorMsgBox(gettext("Unknown status: ") + status);
      break;
    }
  }

  function setupSeverityValue(severity) {
    var severitySelector = $("#selectTriggerSeverity");
    switch(severity) {
    case null:
      severitySelector.val("ANY");
      break;
    case hatohol.TRIGGER_SEVERITY_UNKNOWN:
      severitySelector.val("UNKNOWN");
      break;
    case hatohol.TRIGGER_SEVERITY_INFO:
      severitySelector.val("INFO");
      break;
    case hatohol.TRIGGER_SEVERITY_WARNING:
      severitySelector.val("WARNING");
      break;
    case hatohol.TRIGGER_SEVERITY_ERROR:
      severitySelector.val("ERROR");
      break;
    case hatohol.TRIGGER_SEVERITY_CRITICAL:
      severitySelector.val("CRITICAL");
      break;
    case hatohol.TRIGGER_SEVERITY_EMERGENCY:
      severitySelector.val("EMERGENCY");
      break;
    default:
      hatoholErrorMsgBox(gettext("Unknown severity: ") + severity);
      break;
    }
    if (severity !== null) {
      $("#selectTriggerSeverityCompType").css("visibility","visible");
    }
  }

  function setupSevertyCompTypeValue(compType) {
    var compTypeSelector = $("#selectTriggerSeverityCompType");
    switch(compType) {
    case hatohol.CMP_EQ:
      compTypeSelector.val("CMP_EQ");
      break;
    case hatohol.CMP_EQ_GT:
      compTypeSelector.val("CMP_EQ_GT");
      break;
    }
  }

  // Fill value for update
  if (self.actionDef) {
    setupSeverityValue(self.actionDef.triggerSeverity);
    setupSevertyCompTypeValue(self.actionDef.triggerSeverityComparatorType);
    setupTriggerStatusValue(self.actionDef.triggerStatus);
    if (self.forIncidentSetting) {
      setupIncidentTracker(self.actionDef.command);
    } else {
      setupCommandType(self.actionDef.type);
      setupTimeOutValue(self.actionDef.timeout);
      setupActionCommand(self.actionDef.command);
      setupWorkingDirectory(self.actionDef.workingDirectory);
    }
    self.setApplyButtonState(true);
  }
};

HatoholAddActionDialog.prototype = Object.create(HatoholDialog.prototype);
HatoholAddActionDialog.prototype.constructor = HatoholAddActionDialog;

HatoholAddActionDialog.prototype.createMainElement = function() {
  var self = this;
  var severityMap = {
  // Status: {"label": Label, "value": Value}
  0: {"label": gettext("Not classified"), "value": "UNKNOWN"},
  1: {"label": gettext("Information"), "value": "INFO"},
  2: {"label": gettext("Warning"), "value": "WARNING"},
  3: {"label": gettext("Average"), "value": "ERROR"},
  4: {"label": gettext("High"), "value": "CRITICAL"},
  5: {"label": gettext("Disaster"), "value": "EMERGENCY"}
  };

  if (self.actionDef) {
    getServersAsync();
    getHostGroupsAsync();
    if (!self.forIncidentSetting) {
      getHostsAsync();
      getTriggersAsync();
    }
  }
  getSeverityAsync();

  var div = $(makeMainDivHTML());
  return div;

  //
  // get server info when updating
  //
  function getServersAsync() {
    new HatoholConnector({
      url: "/server",
      replyCallback: replyServerCallback,
      parseErrorCallback: hatoholErrorMsgBoxForParser
    });
  }

  function getHostsAsync() {
    new HatoholConnector({
      url: "/host",
      replyCallback: replyHostCallback,
      parseErrorCallback: hatoholErrorMsgBoxForParser
    });
  }

  function getHostGroupsAsync() {
    new HatoholConnector({
      url: "/hostgroup",
      replyCallback: replyHostGroupCallback,
      parseErrorCallback: hatoholErrorMsgBoxForParser
    });
  }

  function getTriggersAsync() {
    new HatoholConnector({
      url: "/trigger",
      replyCallback: replyTriggerCallback,
      parseErrorCallback: hatoholErrorMsgBoxForParser
    });
  }

  function getSeverityAsync() {
    new HatoholConnector({
      url: "/severity-rank",
      replyCallback: replySeverityCallback,
      parseErrorCallback: hatoholErrorMsgBoxForParser
    });
  }

  function appendSelectElem(selector, infoId) {
    if (infoId) {
      var label = "== " + gettext("SELECT") + " ==";
      selector.append($("<option>").html(label).val("SELECT"));
    }
  }

  function replyServerCallback(reply, parser) {
    if (!(reply.servers instanceof Array)) {
      hatoholErrorMsgBox("[Malformed reply] Not found array: servers");
      return;
    }

    appendSelectElem($("#selectHostgroupId"), self.actionDef.serverId);
    appendSelectElem($("#selectHostId"), self.actionDef.serverId);

    for (var i = 0; i < reply.servers.length; i++) {
      if (reply.servers[i].id != self.actionDef.serverId)
        continue;

      var serverInfo = reply.servers[i];
      var hostName = serverInfo.hostName;
      if (!hostName) {
        hatoholErrorMsgBox("[Malformed reply] Not found element: hostName");
        return;
      }
      var serverId = serverInfo.id;
      if (serverId === undefined) {
        hatoholErrorMsgBox("[Malformed reply] Not found element: id");
        return;
      }

      var displayName = serverId + ": " + hostName;
      $('#selectServerId').append($('<option>').html(displayName).val(serverId));
    }

    setSelectedIdForUpdate($("#selectServerId"), self.actionDef.serverId);
  }

  function replyHostCallback(reply, parser) {
    if (!(reply.hosts instanceof Array)) {
      hatoholErrorMsgBox("[Malformed reply] Not found array: hosts");
      return;
    }

    appendSelectElem($("#selectTriggerId"), self.actionDef.hostId);

    for (var i = 0; i < reply.hosts.length; i ++) {
      if (reply.hosts[i].id != self.actionDef.hostId)
        continue;

      var hostInfo = reply.hosts[i];
      var hostName = hostInfo.hostName;
      if (!hostName) {
        hatoholErrorMsgBox("[Malformed reply] Not found element: hostName");
        return;
      }
      var hostId = hostInfo.id;
      if (hostId === undefined) {
        hatoholErrorMsgBox("[Malformed reply] Not found element: id");
        return;
      }

      var displayName = hostName;
      $('#selectHostId').append($('<option>').html(displayName).val(hostId));
    }

    setSelectedIdForUpdate($("#selectHostId"), self.actionDef.hostId);
  }

  function replyHostGroupCallback(reply, parser) {
    if (!(reply.hostgroups instanceof Array)) {
      hatoholErrorMsgBox("[Malformed reply] Not found array: hostgroups");
      return;
    }

    appendSelectElem($("#selectHostId"), self.actionDef.hostgroupId);

    for (var i = 0; i < reply.hostgroups.length; i ++) {
      if (reply.hostgroups[i].groupId != self.actionDef.hostgroupId)
        continue;

      var hostgroupInfo = reply.hostgroups[i];
      var hostgroupName = hostgroupInfo.groupName;
      if (!hostgroupName) {
        hatoholErrorMsgBox("[Malformed reply] Not found element: hostgroupName");
        return;
      }
      var hostgroupId = hostgroupInfo.groupId;
      if (hostgroupId === undefined) {
        hatoholErrorMsgBox("[Malformed reply] Not found element: hostgroupId");
        return;
      }

      var displayName = hostgroupName;
      $('#selectHostgroupId').append($('<option>').html(displayName).val(hostgroupId));
    }

    setSelectedIdForUpdate($("#selectHostgroupId"), self.actionDef.hostgroupId);
  }

  function replyTriggerCallback(reply, parser) {
    if (!(reply.triggers instanceof Array)) {
      hatoholErrorMsgBox("[Malformed reply] Not found array: triggers");
      return;
    }

    for (var i = 0; i < reply.triggers.length; i ++) {
      if (reply.triggers[i].id != self.actionDef.triggerId)
        continue;

      var triggerInfo = reply.triggers[i];
      var triggerBrief = triggerInfo.brief;
      if (!triggerBrief) {
        hatoholErrorMsgBox("[Malformed reply] Not found element: brief");
        return;
      }
      var triggerId = triggerInfo.id;
      if (triggerId === undefined) {
        hatoholErrorMsgBox("[Malformed reply] Not found element: id");
        return;
      }

      var displayName = triggerBrief;
      $('#selectTriggerId').append($('<option>').html(displayName).val(triggerId));
    }

    setSelectedIdForUpdate($("#selectTriggerId"), self.actionDef.triggerId);
  }

  function replySeverityCallback(reply, parser) {
    if (!(reply.SeverityRanks instanceof Array)) {
      hatoholErrorMsgBox("[Malformed reply] Not found array: SeverityRanks");
      return;
    }

    for (var i = 0; i < reply.SeverityRanks.length; i ++) {
      var severityRank = reply.SeverityRanks[i];

      var severityRankLabel = severityRank.label;
      if (severityRankLabel === undefined) {
        hatoholErrorMsgBox("[Malformed reply] Not found element: label");
        return;
      }
      var severityRankStatus = severityRank.status;
      if (severityRankStatus === undefined) {
        hatoholErrorMsgBox("[Malformed reply] Not found element: status");
        return;
      }

      if (severityRankLabel){
        severityMap[severityRankStatus].label = severityRankLabel;
      }

      var displayName = severityMap[severityRankStatus].label;
      var value = severityMap[severityRankStatus].value;
      $('#selectTriggerSeverity').append($('<option>').html(displayName).val(value));
    }
    if (self.actionDef) {
      setSelectedIdForUpdate($("#selectTriggerSeverity"),
                             severityMap[self.actionDef.triggerSeverity].value);
    }
  }

  function setSelectedIdForUpdate(jQObjSelectId, selectedIdIndex) {
    if (selectedIdIndex) {
      var selectElem = jQObjSelectId;
      selectElem.val(selectedIdIndex);
    }
  }

  function makeTriggerConditionArea() {
    var s = "";
    s += '<h3>' + gettext("Condition") + '</h3>';
    s += '<form class="form-inline">';
    s += '  <label>' + gettext("Server") + '</label>';
    s += '  <select id="selectServerId">';
    s += '    <option value="ANY">ANY</option>';
    s += '    <option value="SELECT">== ' + gettext("SELECT") + ' ==</option>';
    s += '  </select>';

    s += '  <label>' + gettext("Hostgroup") + '</label>';
    s += '  <select id="selectHostgroupId">';
    s += '    <option value="ANY">ANY</option>';
    s += '  </select>';

    if (!self.forIncidentSetting) {
      s += '  <label>' + gettext("Host") + '</label>';
      s += '  <select id="selectHostId">';
      s += '    <option value="ANY">ANY</option>';
      s += '  </select>';

      s += '  <label>' + gettext("Trigger") + '</label>';
      s += '  <select id="selectTriggerId">';
      s += '    <option value="ANY">ANY</option>';
      s += '  </select>';
    }
    s += '</form>';

    s += '<form class="form-inline">';
    s += '  <label for="selectTriggerStatus">' + gettext("Status") + '</label>';
    s += '  <select id="selectTriggerStatus">';
    s += '    <option value="ANY">ANY</option>';
    s += '    <option value="TRIGGER_STATUS_OK">' + gettext("OK") + '</option>';
    s += '    <option value="TRIGGER_STATUS_PROBLEM">' + gettext("Problem") + '</option>';
    s += '  </select>';

    s += '  <label>' + gettext("Severity") + '</label>';
    s += '  <select id="selectTriggerSeverity">';
    s += '    <option value="ANY">ANY</option>';
    s += '  </select>';

    s += '  <select id="selectTriggerSeverityCompType" style="visibility:hidden;">';
    s += '    <option value="CMP_EQ">' + gettext("Equal to") + '</option>';
    s += '    <option value="CMP_EQ_GT">' + gettext("Equal to or greater than") + '</option>';
    s += '  </select>';
    s += '</form>';
    return s;
  }

  function makeExecutionParameterArea() {
    var s = "";
    s += '<h3>' + gettext("Execution parameters") + '</h3>';
    s += '<form class="form-inline">';
    s += '  <label>' + gettext("Templates ") + '</label>';
    s += '  <input id="actor-mail-dialog-button" type="button" value=' + gettext("e-mail") + ' />';
    s += '</form>';

    s += '<form class="form-inline">';
    s += '  <label>' + gettext("Type") + '</label>';
    s += '  <select id="selectType">';
    s += '    <option value="ACTION_COMMAND">' + gettext("COMMAND") + '</option>';
    s += '    <option value="ACTION_RESIDENT">' + gettext("RESIDENT") + '</option>';
    s += '  </select>';

    s += '  <label for="inputTimeout">' + gettext("Time-out (sec)") + '</label>';
    s += '  <input id="inputTimeout" type="text" value="0">';
    s += '  <label for="inputTimeout">(0: ' + gettext("No limit") + ') </label>';
    s += '</form>';

    s += '<form class="form-inline">';
    s += '  <label for="inputActionCommand">' + gettext("Command parameter") + '</label>';
    s += '  <input id="inputActionCommand" type="text" style="width:100%;" value="">';
    s += '</form>';

    s += '<form class="form-inline">';
    s += '  <label for="inputWorkingDir">' + gettext("Execution directory") + '</label>';
    s += '  <input id="inputWorkingDir" type="text" value="" style="width:100%;">';
    s += '</form>';
    return s;
  }

  function makeIncidentTrackerArea() {
    var s = "";
    s += '<h3>' + gettext("Incident Tracking Server") + '</h3>';
    s += '<form class="form-inline">';
    s += '<select id="selectIncidentTracker">';
    s += '</select>';
    s += '<input id="editIncidentTrackers" type="button" ';
    s += '       style="margin-left: 2px;" ';
    s +='        value="' + gettext('EDIT') + '" />';
    s += '</form>';
    return s;
  }

  function makeMainDivHTML() {
    var s = "";
    s += '<div id="add-action-div">';
    s += makeTriggerConditionArea();
    if (self.forIncidentSetting)
      s += makeIncidentTrackerArea();
    else
      s += makeExecutionParameterArea();
    s += '</div>';
    return s;
  }
};

HatoholAddActionDialog.prototype.getCommand = function() {
  if (this.forIncidentSetting)
    return $("#selectIncidentTracker").val();
  else
    return $("#inputActionCommand").val();
};

HatoholAddActionDialog.prototype.updateIncidentTrackers = function(incidentTrackers) {
  var label, incidentTracker;

  if (!this.forIncidentSetting)
    return;

  $("#selectIncidentTracker").empty();
  for (i = 0; i < incidentTrackers.length; i++) {
    incidentTracker = incidentTrackers[i];
    label = "" + incidentTracker.id + ": " + incidentTracker.nickname;
    switch (incidentTracker.type) {
    case hatohol.INCIDENT_TRACKER_REDMINE:
      label += " (" + gettext("Project: ") + incidentTracker.projectId;
      break;
    case hatohol.INCIDENT_TRACKER_HATOHOL:
      label += " (" + gettext("Hatohol");
      break;
    default:
      label += " (" + gettext("Unknown");
    }
    if (incidentTracker.trackerId)
      label += ", " + gettext("Tracker: ") + incidentTracker.trackerId;
    label += ")";
    $("#selectIncidentTracker").append(
      $("<option>", {
        value: incidentTracker.id,
        text: label,
      })
    );
  }
};

HatoholAddActionDialog.prototype.setupIncidentTrackersEditor = function()
{
  var self = this;
  var changedCallback = function(incidentTrackers) {
    var command = $("#selectIncidentTracker").val();
    self.incidentTrackers = incidentTrackers;
    self.updateIncidentTrackers(incidentTrackers);
    if (self.changedCallback)
      self.changedCallback();
    self.setApplyButtonState(!!self.getCommand());
    if (command)
      $("#selectIncidentTracker").val(command);
  };
  $("#editIncidentTrackers").click(function() {
    new HatoholIncidentTrackersEditor({
      changedCallback: changedCallback,
    });
  });
  $("#selectIncidentTracker").change(function() {
    self.setApplyButtonState(!!self.getCommand());
  });
  changedCallback(self.incidentTrackers);
};

HatoholAddActionDialog.prototype.onAppendMainElement = function() {
  var self = this;

  $("#actor-mail-dialog-button").click(function() {
    var currCommand = $("#inputActionCommand").val();
    new HatoholActorMailDialog(applyCallback, currCommand);
  });

  $("#inputActionCommand").keyup(function() {
    fixupApplyButtonState();
  });

  function applyCallback(type, commandDesc) {
    switch (type) {
    case hatohol.ACTION_COMMAND:
    case hatohol.ACTION_RESIDENT:
      $("#selectType").val(type);
      break;
    default:
      var msg = gettext("The template script returned the invalid value.") +
        " (command type: " + type + ")";
      hatoholErrorMsgBox(gettext(msg));
      return;
    }
    $("#inputActionCommand").val(commandDesc);
    fixupApplyButtonState();
  }

  function fixupApplyButtonState() {
    if ($("#inputActionCommand").val())
      self.setApplyButtonState(true);
    else
      self.setApplyButtonState(false);
  }

  if (self.forIncidentSetting)
    self.setupIncidentTrackersEditor();
};

HatoholAddActionDialog.prototype.setApplyButtonState = function(state) {
  var btn = $(".ui-dialog-buttonpane").find("button:contains(" +
            this.applyButtonTitle + ")");
  if (state) {
     btn.removeAttr("disabled");
     btn.removeClass("ui-state-disabled");
  } else {
     btn.attr("disabled", "disabled");
     btn.addClass("ui-state-disabled");
  }
};
