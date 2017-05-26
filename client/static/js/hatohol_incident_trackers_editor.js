/*
 * Copyright (C) 2014 Project Hatohol
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

/*
 * HatoholIncidentTrackersEditor
 */
var HatoholIncidentTrackersEditor = function(params) {
  var self = this;
  var dialogButtons = [{
    text: gettext("CLOSE"),
    click: closeButtonClickedCb
  }];
  self.mainTableId = "incidentTrackersEditorMainTable";
  self.changedCallback = params.changedCallback;
  self.incidentTrackersData = null;

  // call the constructor of the super class
  var dialogAttrs = { width: "800" };
  HatoholDialog.apply(
    this, ["incident-trackers-editor", gettext("EDIT INCIDENT TRACKING SERVERS"),
           dialogButtons, dialogAttrs]);

  //
  // Dialog button handlers
  //
  function closeButtonClickedCb() {
    self.closeDialog();
    if (self.changed && self.changedCallback)
      self.changedCallback(self.incidentTrackersData.incidentTrackers);
  }

  function deleteIncidentTrackers() {
    var deleteList = [], id;
    var checkboxes = $(".incidentTrackerSelectCheckbox");
    $(this).dialog("close");
    for (var i = 0; i < checkboxes.length; i++) {
      if (!checkboxes[i].checked)
        continue;
      id = checkboxes[i].getAttribute("incidentTrackerId");
      deleteList.push(id);
    }
    new HatoholItemRemover({
      id: deleteList,
      type: "incident-tracker",
      completionCallback: function() {
        self.load();
        self.changed = true;
      }
    });
    hatoholInfoMsgBox(gettext("Deleting..."));
  }

  $("#addIncidentTrackerButton").click(function() {
    new HatoholIncidentTrackerEditor({
      succeededCallback: function() {
        self.load();
        self.changed = true;
      },
    });
  });

  $("#deleteIncidentTrackersButton").click(function() {
    var msg = gettext("Delete incident tracking servers ?");
    hatoholNoYesMsgBox(msg, deleteIncidentTrackers);
  });

  self.load();
};

HatoholIncidentTrackersEditor.prototype = Object.create(HatoholDialog.prototype);
HatoholIncidentTrackersEditor.prototype.constructor = HatoholIncidentTrackersEditor;

HatoholIncidentTrackersEditor.prototype.load = function() {
  var self = this;

  new HatoholConnector({
    url: "/incident-tracker",
    request: "GET",
    data: {},
    replyCallback: function(incidentTrackersData, parser) {
      self.incidentTrackersData = incidentTrackersData;
      self.updateMainTable();
    },
    parseErrorCallback: hatoholErrorMsgBoxForParser,
    connectErrorCallback: function(XMLHttpRequest, textStatus, errorThrown) {
      var errorMsg = "Error: " + XMLHttpRequest.status + ": " +
                     XMLHttpRequest.statusText;
      hatoholErrorMsgBox(errorMsg);
    }
  });
};

HatoholIncidentTrackersEditor.prototype.updateMainTable = function() {
  var self = this;
  const setupCheckboxes = function()
  {
    $(".deleteIncidentTracker").shiftcheckbox();
    $(".deleteIncidentTracker").show();
    $(".incidentTrackerSelectCheckbox").change(function()
    {
      const selected = $(".incidentTrackerSelectCheckbox:checked");
      $("#deleteIncidentTrackersButton").attr("disabled", !selected.length);
    });
  };
  const setupEditButtons = function()
  {
    const incidentTrackers = self.incidentTrackersData.incidentTrackers;
    let incidentTrackersMap = {};

    for (let incidentTracker of incidentTrackers)
    {
      incidentTrackersMap[incidentTracker.id] = incidentTracker;
    }

    for (let incidentTracker of incidentTrackers)
    {
      const id = "#editIncidentTracker" + incidentTracker.id;
      $(id).click(function()
      {
        const incidentTrackerId = this.getAttribute("incidentTrackerId");
        new HatoholIncidentTrackerEditor(
        {
          succeededCallback: function()
          {
            self.load();
            self.changed = true;
          },
          incidentTracker: incidentTrackersMap[incidentTrackerId],
        });
      });
    }
  };

  if (!this.incidentTrackersData)
    return;

  var rows = this.generateTableRows(this.incidentTrackersData);
  var tbody = $("#" + this.mainTableId + " tbody");
  tbody.empty().append(rows);
  setupCheckboxes();
  setupEditButtons();
};

HatoholIncidentTrackersEditor.prototype.generateMainTable = function() {
  var html =
  '<form class="form-inline">' +
  '  <input id="addIncidentTrackerButton" type="button" ' +
  '         class="addIncidentTracker form-control"' +
  '         value="' + gettext("ADD") + '" />' +
  '  <input id="deleteIncidentTrackersButton" type="button" disabled ' +
  '         class="deleteIncidentTracker form-control" ' +
  '         value="' + gettext("DELETE") + '" />' +
  '</form>' +
  '<div class="ui-widget-content" style="overflow-y: auto;">' +
  '<table class="table table-condensed table-striped table-hover" id=' +
  this.mainTableId + '>' +
  '  <thead>' +
  '    <tr>' +
  '      <th class="deleteIncidentTracker"> </th>' +
  '      <th>' + gettext("ID") + '</th>' +
  '      <th>' + gettext("Type") + '</th>' +
  '      <th>' + gettext("Nickname") + '</th>' +
  '      <th>' + gettext("Base URL") + '</th>' +
  '      <th>' + gettext("Project ID") + '</th>' +
  '      <th>' + gettext("Tracker ID") + '</th>' +
  '      <th></th>' +
  '    </tr>' +
  '  </thead>' +
  '  <tbody></tbody>' +
  '</table>' +
  '</div>';
  return html;
};

HatoholIncidentTrackersEditor.prototype.generateTableRows = function(data) {
  let html = '';

  for (let tracker of data.incidentTrackers)
  {
    let type = gettext("Unknown");
    if (tracker.type == hatohol.INCIDENT_TRACKER_REDMINE)
    {
      type = gettext("Redmine");
    }
    else if (tracker.type == hatohol.INCIDENT_TRACKER_HATOHOL)
    {
      type = gettext("Hatohol");
    }
    html +=
    '<tr>' +
    '<td class="deleteIncidentTracker">' +
    '  <input type="checkbox" class="incidentTrackerSelectCheckbox"' +
    '     value="' + data.incidentTrackers.indexOf(tracker) + '"' +
    '     incidentTrackerId="' + escapeHTML(tracker.id) + '"></td>' +
    '<td>' + escapeHTML(tracker.id) + '</td>' +
    '<td>' + type + '</td>' +
    '<td>' + escapeHTML(tracker.nickname) + '</td>' +
    '<td>' + escapeHTML(tracker.baseURL) + '</td>' +
    '<td>' + escapeHTML(tracker.projectId) + '</td>' +
    '<td>' + escapeHTML(tracker.trackerId) + '</td>' +
    '<td>' +
    '<form class="form-inline" style="margin: 0">' +
    '  <input id="editIncidentTracker' + escapeHTML(tracker.id) + '"' +
    '    type="button" class="btn btn-default editIncidentTracker"' +
    '    incidentTrackerId="' + escapeHTML(tracker.id) + '"' +
    '    value="' + gettext("Edit") + '" />' +
    '</form>' +
    '</td>' +
    '</tr>';
  }
  return html;
};

HatoholIncidentTrackersEditor.prototype.createMainElement = function() {
  var self = this;
  var element = $(this.generateMainTable());
  return element;
};



/*
 * HatoholIncidentTrackerEditor
 */
var HatoholIncidentTrackerEditor = function(params) {
  var self = this;

  self.incidentTracker = params.incidentTracker;
  self.succeededCallback = params.succeededCallback;
  self.windowTitle = self.incidentTracker ?
    gettext("EDIT INCIDENT TRACKING SERVER") :
    gettext("ADD INCIDENT TRACKING SERVER");
  self.applyButtonTitle = self.incidentTracker ?
    gettext("APPLY") : gettext("ADD");

  var dialogButtons = [{
    text: self.applyButtonTitle,
    click: applyButtonClickedCb
  }, {
    text: gettext("CANCEL"),
    click: cancelButtonClickedCb
  }];

  // call the constructor of the super class
  dialogAttrs = { width: "auto" };
  HatoholDialog.apply(
    this, ["incident-tracker-editor", self.windowTitle,
           dialogButtons, dialogAttrs]);

  //
  // Dialog button handlers
  //
  function applyButtonClickedCb() {
    if (validateParameters()) {
      makeQueryData();
      if (self.incidentTracker)
        hatoholInfoMsgBox(
          gettext("Now updating the incident tracking server ..."));
      else
        hatoholInfoMsgBox(
          gettext("Now creating an incident tracking server ..."));
      postIncidentTracker();
    }
  }

  function makeQueryData() {
    var queryData = {};
    queryData.type = $("#selectIncidentTrackerType").val();
    queryData.nickname = $("#editIncidentTrackerNickname").val();
    queryData.baseURL = $("#editIncidentTrackerBaseURL").val();
    queryData.projectId = $("#editIncidentTrackerProjectId").val();
    queryData.trackerId = $("#editIncidentTrackerTrackerId").val();
    queryData.userName = $("#editIncidentTrackerUserName").val();
    if ($("#editIncidentTrackerPasswordCheckbox").is(":checked"))
      queryData.password = $("#editIncidentTrackerPassword").val();
    return queryData;
  }

  function postIncidentTracker() {
    var url = "/incident-tracker";
    if (self.incidentTracker)
      url += "/" + self.incidentTracker.id;
    new HatoholConnector({
      url: url,
      request: self.incidentTracker ? "PUT" : "POST",
      data: makeQueryData(),
      replyCallback: replyCallback,
      parseErrorCallback: hatoholErrorMsgBoxForParser
    });
  }

  function replyCallback(reply, parser) {
    self.closeDialog();
    if (self.incidentTracker)
      hatoholInfoMsgBox(gettext("Successfully updated."));
    else
      hatoholInfoMsgBox(gettext("Successfully created."));

    if (self.succeededCallback)
      self.succeededCallback();
  }

  function validateParameters() {
    var label;

    if($("#selectIncidentTrackerType").val() == hatohol.INCIDENT_TRACKER_HATOHOL) {
      return true;
    }

    if ($("#editIncidentTrackerNickname").val() === "") {
      hatoholErrorMsgBox(gettext("Nickname is empty!"));
      return false;
    }

    if ($("#editIncidentTrackerBaseURL").val() === "") {
      hatoholErrorMsgBox(gettext("Base URL is empty!"));
      return false;
    }

    if ($("#editIncidentTrackerProjectId").val() === "") {
      hatoholErrorMsgBox(gettext("Project ID is empty!"));
      return false;
    }

    if ($("#editIncidentTrackerUserName").val() === "") {
      label = $("label[for=editIncidentTrackerUserName]").text();
      hatoholErrorMsgBox(label + gettext(" is empty!"));
      return false;
    }

    return true;
  }

  function cancelButtonClickedCb() {
    self.closeDialog();
  }
};

HatoholIncidentTrackerEditor.prototype = Object.create(HatoholDialog.prototype);
HatoholIncidentTrackerEditor.prototype.constructor = HatoholIncidentTrackerEditor;

HatoholIncidentTrackerEditor.prototype.createMainElement = function() {
  var html = '<div>';

  html +=
  '<div>' +
  '<label>' + gettext("Type") + '</label>' +
  '<select id="selectIncidentTrackerType" style="width:10em">' +
  '  <option value="' + hatohol.INCIDENT_TRACKER_REDMINE + '">' +
    gettext("Redmine") + '</option>' +
  '  <option value="' + hatohol.INCIDENT_TRACKER_HATOHOL + '">' +
    gettext("Hatohol") + '</option>' +
  '</select>' +
  '</div>' +
  '<div id="editIncidentTrackerArea">' +
  '<div>' +
  '<label for="editIncidentTrackerNickname">' + gettext("Nickname") + '</label>' +
  '<input id="editIncidentTrackerNickname" type="text" ' +
  '       class="input-xlarge">' +
  '</div>' +
  '<div>' +
  '<label for="editIncidentTrackerBaseURL">' + gettext("Base URL") + '</label>' +
  '<input id="editIncidentTrackerBaseURL" type="text" ' +
  '       class="input-xlarge">' +
  '</div>' +
  '<div>' +
  '<label for="editIncidentTrackerProjectId">' + gettext("Project ID") + '</label>' +
  '<input id="editIncidentTrackerProjectId" type="text" ' +
  '       class="input-xlarge">' +
  '</div>' +
  '<div>' +
  '<label for="editIncidentTrackerProjectId">' + gettext("Tracker ID") + '</label>' +
  '<input id="editIncidentTrackerTrackerId" type="text" ' +
  '       class="input-xlarge" ' +
  '       placeholder="' + gettext("(empty: Default)") + '">' +
  '</div>' +
  '<div>' +
  '<label for="editIncidentTrackerUserName">' + gettext("User name") + '</label>' +
  '<input id="editIncidentTrackerUserName" type="text" ' +
  '       class="input-xlarge">' +
  '</div>' +
  '<div id="editIncidentTrackerPasswordArea">' +
  '<label for="editIncidentTrackerPassword">' + gettext("Password") + '</label>' +
  '<input id="editIncidentTrackerPassword" type="password" ' +
  '       class="input-xlarge">' +
  '<input type="checkbox" id="editIncidentTrackerPasswordCheckbox"> ' +
  '</div>' +
  '</div>';

  return html;
};

HatoholIncidentTrackerEditor.prototype.onAppendMainElement = function() {
  this.setIncidentTracker(this.incidentTracker);
  this.resetWidgetState();

  this.applyEditIncidentTrackerAreaState();
  this.toggleEditIncidentTrackerArea();
};

HatoholIncidentTrackerEditor.prototype.toggleEditIncidentTrackerArea = function() {
  var self = this;
  $("#selectIncidentTrackerType").change(function() {
    self.applyEditIncidentTrackerAreaState();
  });
};

HatoholIncidentTrackerEditor.prototype.applyEditIncidentTrackerAreaState = function() {
  if ($("#selectIncidentTrackerType").val() == hatohol.INCIDENT_TRACKER_HATOHOL)
    $("#editIncidentTrackerArea").hide();
  else
    $("#editIncidentTrackerArea").show();
};

HatoholIncidentTrackerEditor.prototype.resetWidgetState = function() {
  // We always use API key for Redmine, and we're currently supporting only
  // Redmine. So we don't need the password entry at this time.
  var editPassword = false;
  var showPasswordEntry = false;
  if (showPasswordEntry) {
    $("label[for=editIncidentTrackerUserName]").text(gettext("User name"));
    $("#editIncidentTrackerPasswordArea").show();
    if (editPassword) {
      $("#editIncidentTrackerPasswordCheckbox").hide();
    } else {
      $("#editIncidentTrackerPasswordCheckbox").show();
    }
  } else {
    $("label[for=editIncidentTrackerUserName]").text(gettext("API Key"));
    $("#editIncidentTrackerPasswordArea").hide();
  }
  $("#editIncidentTrackerPasswordCheckbox").prop("checked", editPassword);
  $("#editIncidentTrackerPassword").attr("disabled", !editPassword);

  $("#editIncidentTrackerPasswordCheckbox").change(function() {
    var check = $(this).is(":checked");
    $("#editIncidentTrackerPassword").attr("disabled", !check);
  });
};

HatoholIncidentTrackerEditor.prototype.setIncidentTracker = function(tracker) {
  this.incidentTracker = tracker;
  $("#selectIncidentTrackerType").val(tracker ? tracker.type : 0);
  $("#editIncidentTrackerNickname").val(tracker ? tracker.nickname : "");
  $("#editIncidentTrackerBaseURL").val(tracker ? tracker.baseURL : "");
  $("#editIncidentTrackerProjectId").val(tracker ? tracker.projectId : "");
  $("#editIncidentTrackerTrackerId").val(tracker ? tracker.trackerId : "");
  $("#editIncidentTrackerUserName").val(tracker ? tracker.userName : "");
  $("#editIncidentTrackerPassword").val("");
};
