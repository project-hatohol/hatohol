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

var HatoholIssueTrackersEditor = function(params) {
  var self = this;
  var dialogButtons = [{
    text: gettext("CLOSE"),
    click: closeButtonClickedCb
  }];

  // call the constructor of the super class
  var dialogAttrs = { width: "600" };
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
};

HatoholIssueTrackersEditor.prototype = Object.create(HatoholDialog.prototype);
HatoholIssueTrackersEditor.prototype.constructor = HatoholIssueTrackersEditor;

HatoholIssueTrackersEditor.prototype.generateMainTable = function() {
  var html =
  '<form class="form-inline">' +
  '  <input id="addIssueTrackerButton" type="button" ' +
  '    class="addIssueTracker form-control" value="' + gettext("ADD") + '" />' +
  '  <input id="deleteIssueTrackersButton" type="button" disabled ' +
  '    class="deleteIssueTracker form-control" value="' + gettext("DELETE") + '" />' +
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
  '    </tr>' +
  '  </thead>' +
  '  <tbody></tbody>' +
  '</table>' +
  '</div>';
  return html;
};

HatoholIssueTrackersEditor.prototype.createMainElement = function() {
  var self = this;
  var element = $(this.generateMainTable());
  return element;
};
