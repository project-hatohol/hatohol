/*
 * Copyright (C) 2014 Project Hatohol
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

/*
 * HatoholIssueTrackersEditor
 */
var HatoholIssueTrackersEditor = function(params) {
  var self = this;

  self.mainTableId = "issueTrackersEditorMainTable";
  self.changedCallback = params.changedCallback;
  self.issueTrackersData = null;

  //
  // Dialog button handlers
  //
  $('#issueTrackersEditor').on('hide.bs.modal', function (e) {
    if (self.changed && self.changedCallback)
      self.changedCallback(self.issueTrackersData.issueTrackers);
  });

  function deleteIssueTrackers() {
    var deleteList = [], id;
    var checkboxes = $(".issueTrackerSelectCheckbox");
    $(this).dialog("close");
    for (var i = 0; i < checkboxes.length; i++) {
      if (!checkboxes[i].checked)
        continue;
      id = checkboxes[i].getAttribute("issueTrackerId");
      deleteList.push(id);
    }
    new HatoholItemRemover({
      id: deleteList,
      type: "issue-tracker",
      completionCallback: function() {
        self.load();
        self.changed = true;
      }
    });
    hatoholInfoMsgBox(gettext("Deleting..."));
  }

  $("#addIssueTrackerButton").click(function() {
    $("#issueTrackerEditor").modal("show");
    new HatoholIssueTrackerEditor({
      succeededCallback: function() {
        self.load();
        self.changed = true;
      },
    });
  });

  $("#deleteIssueTrackersButton").click(function() {
    var msg = gettext("Are you sure to delete issue tracking servers?");
    hatoholNoYesMsgBox(msg, deleteIssueTrackers);
  });

  self.load();
};

HatoholIssueTrackersEditor.prototype.load = function() {
  var self = this;

  new HatoholConnector({
    url: "/issue-tracker",
    request: "GET",
    data: {},
    replyCallback: function(issueTrackersData, parser) {
      self.issueTrackersData = issueTrackersData;
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

HatoholIssueTrackersEditor.prototype.updateMainTable = function() {
  var self = this;
  var numSelected = 0;
  var setupCheckboxes = function() {
    $(".issueTrackerSelectCheckbox").change(function() {
      var check = $(this).is(":checked");
      var prevNumSelected = numSelected;
      if (check)
        numSelected += 1;
      else
        numSelected -= 1;
      if (prevNumSelected == 0 && numSelected == 1)
        $("#deleteIssueTrackersButton").attr("disabled", false);
      else if (prevNumSelected == 1 && numSelected == 0)
        $("#deleteIssueTrackersButton").attr("disabled", true);
    });
  };
  var setupEditButtons = function()
  {
    var issueTrackers = self.issueTrackersData.issueTrackers;
    var issueTrackersMap = {};
    var i, id;

    for (i = 0; i < issueTrackers.length; ++i)
      issueTrackersMap[issueTrackers[i]["id"]] = issueTrackers[i];

    for (i = 0; i < issueTrackers.length; ++i) {
      id = "#editIssueTracker" + issueTrackers[i]["id"];
      $(id).click(function() {
        var issueTrackerId = this.getAttribute("issueTrackerId");
	$('#issueTrackerEditor').modal("show");
        new HatoholIssueTrackerEditor({
          succeededCallback: function() {
            self.load();
            self.changed = true;
          },
          issueTracker: issueTrackersMap[issueTrackerId],
        });
      });
    }
  };

  if (!this.issueTrackersData)
    return;

  var rows = this.generateTableRows(this.issueTrackersData);
  var tbody = $("#" + this.mainTableId + " tbody");
  tbody.empty().append(rows);
  setupCheckboxes();
  setupEditButtons();
};

HatoholIssueTrackersEditor.prototype.generateTableRows = function(data) {
  var html = '';
  var tracker, type;

  for (var i = 0; i < data.issueTrackers.length; i++) {
    tracker = data.issueTrackers[i];
    type = tracker.type == 0 ? gettext("Redmine") : gettext("Unknown");
    html +=
    '<tr>' +
    '<td class="deleteIssueTracker">' +
    '  <input type="checkbox" class="issueTrackerSelectCheckbox" ' +
    '     issueTrackerId="' + escapeHTML(tracker.id) + '"></td>' +
    '<td>' + escapeHTML(tracker.id) + '</td>' +
    '<td>' + type + '</td>' +
    '<td>' + escapeHTML(tracker.nickname) + '</td>' +
    '<td>' + escapeHTML(tracker.baseURL) + '</td>' +
    '<td>' + escapeHTML(tracker.projectId) + '</td>' +
    '<td>' + escapeHTML(tracker.trackerId) + '</td>' +
    '<td>' +
    '<form class="form-inline" style="margin: 0">' +
    '  <input id="editIssueTracker' + escapeHTML(tracker["id"]) + '"' +
    '    type="button" class="btn btn-default editIssueTracker"' +
    '    issueTrackerId="' + escapeHTML(tracker["id"]) + '"' +
    '    value="' + gettext("Edit") + '" />' +
    '</form>' +
    '</td>' +
    '</tr>';
  }
  return html;
};



/*
 * HatoholIssueTrackerEditor
 */
var HatoholIssueTrackerEditor = function(params) {
  var self = this;

  self.setIssueTracker(params.issueTracker);
  self.succeededCallback = params.succeededCallback;
  self.windowTitle = self.issueTracker ?
    gettext("EDIT ISSUE TRACKING SERVER") :
    gettext("ADD ISSUE TRACKING SERVER");

  //
  // Dialog button handlers
  //
  //function applyButtonClickedCb() {
  $("#saveIssueTrackerButton").click(function() {
    if (validateParameters()) {
      makeQueryData();
      hatoholInfoMsgBox(
	gettext("Now saving the issue tracking server ..."));
      postIssueTracker();
    }
  });

  function makeQueryData() {
    var queryData = {};
    queryData.type = $("#selectIssueTrackerType").val()
    queryData.nickname = $("#editIssueTrackerNickname").val();
    queryData.baseURL = $("#editIssueTrackerBaseURL").val();
    queryData.projectId = $("#editIssueTrackerProjectId").val();
    queryData.trackerId = $("#editIssueTrackerTrackerId").val();
    queryData.userName = $("#editIssueTrackerUserName").val();
    if ($("#editPasswordCheckbox").is(":checked"))
      queryData.password = $("#editIssueTrackerPassword").val();
    return queryData;
  }

  function postIssueTracker() {
    var url = "/issue-tracker";
    if (self.issueTracker)
      url += "/" + self.issueTracker.id;
    new HatoholConnector({
      url: url,
      request: self.issueTracker ? "PUT" : "POST",
      data: makeQueryData(),
      replyCallback: replyCallback,
      parseErrorCallback: hatoholErrorMsgBoxForParser
    });
  }

  function replyCallback(reply, parser) {
    $("#issueTrackerEditor").modal("hide");
    if (self.issueTracker)
      hatoholInfoMsgBox(gettext("Successfully updated."));
    else
      hatoholInfoMsgBox(gettext("Successfully created."));

    if (self.succeededCallback)
      self.succeededCallback();
  }

  function validateParameters() {
    if ($("#editIssueTrackerNickname").val() == "") {
      hatoholErrorMsgBox(gettext("Nickname is empty!"));
      return false;
    }

    if ($("#editIssueTrackerBaseURL").val() == "") {
      hatoholErrorMsgBox(gettext("Base URL is empty!"));
      return false;
    }

    if ($("#editIssueTrackerProjectId").val() == "") {
      hatoholErrorMsgBox(gettext("Project ID is empty!"));
      return false;
    }

    if ($("#editIssueTrackerUserName").val() == "") {
      hatoholErrorMsgBox(gettext("User name is empty!"));
      return false;
    }

    return true;
  }

  function cancelButtonClickedCb() {
    self.closeDialog();
  }
};

HatoholIssueTrackerEditor.prototype.setIssueTracker = function(tracker) {
  this.issueTracker = tracker;

  $("#editIssueTrackerNickname").val(tracker ? tracker.nickname : "");
  $("#editIssueTrackerBaseURL").val(tracker ? tracker.baseURL : "");
  $("#editIssueTrackerProjectId").val(tracker ? tracker.projectId : "");
  $("#editIssueTrackerTrackerId").val(tracker ? tracker.trackerId : "");
  $("#editIssueTrackerUserName").val(tracker ? tracker.userName : "");
};

HatoholIssueTrackerEditor.prototype.onAppendMainElement = function () {
  var editPassword = !this.issueTracker;
  if (editPassword) {
    $("#editPasswordCheckbox").hide();
  } else {
    $("#editPasswordCheckbox").show();
  }
  $("#editPasswordCheckbox").prop("checked", editPassword);
  $("#editIssueTrackerPassword").attr("disabled", !editPassword);

  $("#editPasswordCheckbox").change(function() {
    var check = $(this).is(":checked");
    $("#editIssueTrackerPassword").attr("disabled", !check);
  });
};
