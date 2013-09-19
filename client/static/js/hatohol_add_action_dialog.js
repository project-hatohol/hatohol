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

var HatoholAddActionDialog = function(addSucceededCb) {
  var self = this;

  var dialogButtons = [{
    text: gettext("ADD"),
    click: addButtonClickedCb,
  }, {
    text: gettext("CANCEL"),
    click: cancelButtonClickedCb,
  }];

  // call the constructor of the super class
  HatoholDialog.apply(
    this, ["add-action-dialog", gettext("ADD ACTION"), dialogButtons]);

  //
  // Dialog button handlers
  //
  function addButtonClickedCb() {
    if (validateAddParameters()) {
      makeQueryData();
      showMessageBox(gettext("Now creating an action ..."),
                     gettext("Information"));
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
    if ($(this).val() == "SELECT")
      new HatoholServerSelector(serverSelectedCb);
  })

  function serverSelectedCb(serverInfo) {
    var numOptions = $("#selectServerId").children().length;
    if (!serverInfo) {
      $("#selectServerId").val("ANY");
      return;
    }
    if (numOptions == 3)
      $("#selectServerId").children('option:last-child').remove();

    var label = serverInfo.id + ": " + serverInfo.hostName;
    var value = serverInfo.id
    $("#selectServerId").append($("<option>").html(label).val(value));
    $("#selectServerId").val(value);
  }

  //
  // General class methods
  //
  function getCommandType() {
    var type = $("#selectType").val();
    switch(type) {
    case "ACTION_COMMAND":
      return ACTION_COMMAND;
    case "ACTION_RESIDENT":
      return ACTION_RESIDENT;
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
      return CMP_EQ;
    case "CMP_EQ_GT":
      return CMP_EQ_GT;
    default:
      alert("Unknown severity: " + severity);
    }
    return undefined;
  }

  function makeQueryData() {
      var queryData = {
        csrfmiddlewaretoken: $("*[name=csrfmiddlewaretoken]").val(),
      };
      var serverId = $("#selectServerId").val();
      if (serverId != "ANY")
        queryData.serverId = serverId;
      queryData.type = getCommandType();
      queryData.timeout = $("#inputTimeout").val();
      queryData.command = $("#inputActionCommand").val();
      queryData.workingDirectory = $("#inputWorkingDir").val();
      queryData.triggerStatus   = getStatusValue();
      queryData.triggerSeverity = getSeverityValue();
      queryData.triggerSeverityCompType = getSeverityCompTypeValue();
      return queryData;
  }

  function postAddAction() {
    $.ajax({
      url: "/tunnel/action",
      type: "POST",
      data: makeQueryData(),
      success: function(data) {
        parsePostActionResult(data);
      },
      error: function(XMLHttpRequest, textStatus, errorThrown) {
        var errorMsg = "Error: " + XMLHttpRequest.status + ": " +
                       XMLHttpRequest.statusText;
        showErrorMessageBox(errorMsg);
      }
    })
  }

  function parsePostActionResult(data) {
    if (!parseResult(data))
      return;

    self.closeDialog();
    showInfoMessageBox(gettext("Successfully created."));

    if (addSucceededCb)
      addSucceededCb();
  }

  function validateAddParameters() {
    if ($("#inputActionCommand").val() == "") {
      showErrorMessageBox("Command is empty!");
      return false;
    }
    return true;
  }
}

HatoholAddActionDialog.prototype = Object.create(HatoholDialog.prototype);
HatoholAddActionDialog.prototype.constructor = HatoholAddActionDialog;

HatoholAddActionDialog.prototype.createMainElement = function() {
  var div = $(makeMainDivHTML());
  return div;

  function makeMainDivHTML() {
    var s = "";
    s += '<div id="add-action-div">'
    s += '<h3>' + gettext("Condition") + '</h3>'
    s += '<form class="form-inline">'
    s += '  <label>' + gettext("Server") + '</label>'
    s += '  <select id="selectServerId">'
    s += '    <option value="ANY">ANY</option>'
    s += '    <option value="SELECT">== ' + gettext("SELECT") + ' ==</option>'
    s += '  </select>'

    s += '  <label>' + gettext("Host Group") + '</label>'
    s += '  <select id="selectHostGroupId">'
    s += '    <option value="ANY">ANY</option>'
    s += '    <!-- Currently host group ID is not supported'
    s += '    <option value="select">== ' + gettext("SELECT") + ' ==</option>'
    s += '    -->'
    s += '  </select>'
    s += '  <label>' + gettext("Host") + '</label>'
    s += '  <select id="selectHostId">'
    s += '    <option value="ANY">ANY</option>'
    s += '    <option value="select">== ' + gettext("SELECT") + ' ==</option>'
    s += '  </select>'
    s += '</form>'

    s += '<form class="form-inline">'
    s += '  <label>' + gettext("Trigger") + '</label>'
    s += '  <select id="selectTriggerId">'
    s += '    <option value="ANY">ANY</option>'
    s += '    <option value="select">== ' + gettext("SELECT") + '==</option>'
    s += '  </select>'

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

    s += '<h3>' + gettext("Execution parameters") + '</h3>'
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
    s += '  <label for="inputActionCommand">' + gettext("Command") + '</label>'
    s += '  <input id="inputActionCommand" type="text" value="" style="width:100%;">'
    s += '</form>'

    s += '<form class="form-inline">'
    s += '  <label for="inputWorkingDir">' + gettext("Execution directory") + '</label>'
    s += '  <input id="inputWorkingDir" type="text" value="" style="width:100%;">'
    s += '</form>'
    s += '</div>'
    return s;
  }
}

