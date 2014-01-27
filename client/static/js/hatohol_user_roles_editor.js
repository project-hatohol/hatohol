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

var HatoholUserRolesEditor = function() {
  var self = this;
  var dialogButtons = [{
    text: gettext("CLOSE"),
    click: closeButtonClickedCb
  }];
  self.mainTableId = "userRoleEditorMainTable";
  self.userRolesData = null;

  // call the constructor of the super class
  dialogAttrs = { width: "auto" };
  HatoholDialog.apply(
    this, ["user-roles-editor", gettext("EDIT USER ROLES"),
           dialogButtons, dialogAttrs]);

  //
  // Dialog button handlers
  //
  function closeButtonClickedCb() {
    self.closeDialog();
  }

  self.start();
};

HatoholUserRolesEditor.prototype = Object.create(HatoholDialog.prototype);
HatoholUserRolesEditor.prototype.constructor = HatoholUserRolesEditor;

HatoholUserRolesEditor.prototype.start = function() {
  var self = this;

  new HatoholConnector({
    url: "/user-role",
    request: "GET",
    data: {},
    replyCallback: function(userRolesData, parser) {
      self.userRolesData = userRolesData;
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

HatoholUserRolesEditor.prototype.updateMainTable = function() {
  if (!this.userRolesData)
    return;

  var rows = this.generateTableRows(this.userRolesData);
  $("#" + this.mainTableId).append(rows);
};

HatoholUserRolesEditor.prototype.generateMainTable = function() {
  var html =
  '<table class="table table-condensed table-striped table-hover" id=' +
  this.mainTableId + '>' +
  '  <thead>' +
  '    <tr>' +
  '      <th> </th>' +
  '      <th>ID</th>' +
  '      <th>' + gettext("Name") + '</th>' +
  '      <th>' + gettext("Flags") + '</th>' +
  '    </tr>' +
  '  </thead>' +
  '  <tbody></tbody>' +
  '</table>';
  return html;
};

HatoholUserRolesEditor.prototype.generateTableRows = function(data) {
  var html = '', role;
  for (var i = 0; i < data.userRoles.length; i++) {
    role = data.userRoles[i];
    html += '<tr>';
    html += '<td><input type="checkbox" class="userRoleSelectCheckbox" ' +
            'userRoleId="' + role.userRoleId + '"></td>';
    html += '<td>' + role.userRoleId + '</td>';
    html += '<td>' + role.name + '</td>';
    html += '<td>' + role.flags + '</td>';
    html += '</tr>';
  }
  return html;
};

HatoholUserRolesEditor.prototype.createMainElement = function() {
  var self = this;
  var element = $(this.generateMainTable());
  return element;
};

HatoholUserRolesEditor.prototype.onAppendMainElement = function () {
};
