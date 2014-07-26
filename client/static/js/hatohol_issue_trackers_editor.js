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
  var dialogButtons = [{
    text: gettext("CLOSE"),
    click: closeButtonClickedCb
  }];
  self.mainTableId = "issueTrackersEditorMainTable";
  self.issueTrackersData = null;

  // call the constructor of the super class
  var dialogAttrs = { width: "700" };
  HatoholDialog.apply(
    this, ["issue-trackers-editor", gettext("EDIT ISSUE TRACKING SERVERS"),
           dialogButtons, dialogAttrs]);

  //
  // Dialog button handlers
  //
  function closeButtonClickedCb() {
    self.closeDialog();
    if (self.changed && self.changedCallback)
      self.changedCallback();
  }

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

HatoholIssueTrackersEditor.prototype = Object.create(HatoholDialog.prototype);
HatoholIssueTrackersEditor.prototype.constructor = HatoholIssueTrackersEditor;

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

HatoholIssueTrackersEditor.prototype.generateMainTable = function() {
  var html =
  '<form class="form-inline">' +
  '  <input id="addIssueTrackerButton" type="button" ' +
  '         class="addIssueTracker form-control"' +
  '         value="' + gettext("ADD") + '" />' +
  '  <input id="deleteIssueTrackersButton" type="button" disabled ' +
  '         class="deleteIssueTracker form-control" ' +
  '         value="' + gettext("DELETE") + '" />' +
  '</form>' +
  '<div class="ui-widget-content" style="overflow-y: auto; height: 200px">' +
  '<table class="table table-condensed table-striped table-hover" id=' +
  this.mainTableId + '>' +
  '  <thead>' +
  '    <tr>' +
  '      <th class="deleteIssueTracker"> </th>' +
  '      <th>' + gettext("ID") + '</th>' +
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

HatoholIssueTrackersEditor.prototype.generateTableRows = function(data) {
  var html = '';
  var tracker;

  for (var i = 0; i < data.issueTrackers.length; i++) {
    tracker = data.issueTrackers[i];
    html +=
    '<tr>' +
    '<td class="deleteIssueTracker">' +
    '  <input type="checkbox" class="issueTrackerSelectCheckbox" ' +
    '     issueTrackerId="' + escapeHTML(tracker.id) + '"></td>' +
    '<td>' + escapeHTML(tracker.id) + '</td>' +
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

HatoholIssueTrackersEditor.prototype.createMainElement = function() {
  var self = this;
  var element = $(this.generateMainTable());
  return element;
};



/*
 * HatoholIssueTrackerEditor
 */
var HatoholIssueTrackerEditor = function(params) {
  var self = this;

  self.issueTracker = params.issueTracker;
  self.windowTitle = self.issueTracker ?
    gettext("EDIT ISSUE TRACKING SERVER") :
    gettext("ADD ISSUE TRACKING SERVER");
  self.applyButtonTitle = self.issueTracker ?
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
    this, ["issue-tracker-editor", self.windowTitle,
           dialogButtons, dialogAttrs]);

  //
  // Dialog button handlers
  //
  function applyButtonClickedCb() {
  }

  function cancelButtonClickedCb() {
    self.closeDialog();
  }
};

HatoholIssueTrackerEditor.prototype = Object.create(HatoholDialog.prototype);
HatoholIssueTrackerEditor.prototype.constructor = HatoholIssueTrackerEditor;

HatoholIssueTrackerEditor.prototype.createMainElement = function() {
  return "";
};
