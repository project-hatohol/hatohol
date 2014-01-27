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
  dialogAttrs = { width: "600" };
  HatoholDialog.apply(
    this, ["user-roles-editor", gettext("EDIT USER ROLES"),
           dialogButtons, dialogAttrs]);

  //
  // Dialog button handlers
  //
  function closeButtonClickedCb() {
    self.closeDialog();
  }

  function deleteUserRoles() {
    var deleteList = [], id;
    var checkboxes = $(".userRoleSelectCheckbox");
    $(this).dialog("close");
    for (var i = 0; i < checkboxes.length; i++) {
      if (!checkboxes[i].checked)
        continue;
      id = checkboxes[i].getAttribute("userRoleId");
      deleteList.push(id);
    }
    new HatoholItemRemover({
      id: deleteList,
      type: "user-role",
      completionCallback: function() {
        self.load();
      }
    });
    hatoholInfoMsgBox(gettext("Deleting..."));
  }

  $("#addUserRoleButton").click(function() {
    var succeededCb = function() {
      self.load();
    };
    new HatoholUserRoleEditor(succeededCb);
  });

  $("#deleteUserRolesButton").click(function() {
    var msg = gettext("Are you sure to delete selected items?");
    hatoholNoYesMsgBox(msg, deleteUserRoles);
  });

  self.load();
};

HatoholUserRolesEditor.prototype = Object.create(HatoholDialog.prototype);
HatoholUserRolesEditor.prototype.constructor = HatoholUserRolesEditor;

HatoholUserRolesEditor.prototype.load = function() {
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
  var numSelected = 0;
  var setupCheckboxHandler = function() {
    $(".userRoleSelectCheckbox").change(function() {
      var check = $(this).is(":checked");
      var prevNumSelected = numSelected;
      if (check)
        numSelected += 1;
      else
        numSelected -= 1;
      if (prevNumSelected == 0 && numSelected == 1)
        $("#deleteUserRolesButton").attr("disabled", false);
      else if (prevNumSelected == 1 && numSelected == 0)
        $("#deleteUserRolesButton").attr("disabled", true);
    });
  };

  if (!this.userRolesData)
    return;

  var rows = this.generateTableRows(this.userRolesData);
  var tbody = $("#" + this.mainTableId + " tbody");
  tbody.empty().append(rows);
  setupCheckboxHandler();
};

HatoholUserRolesEditor.prototype.generateMainTable = function() {
  var html =
  '<form class="form-inline">' +
  '  <input id="addUserRoleButton" type="button" class="btn"' +
  '   value="' + gettext("ADD") + '" />' +
  '  <input id="deleteUserRolesButton" type="button" class="btn" disabled' +
  '   value="' + gettext("DELETE") + '" />' +
  '</form>' +
  '<table class="table table-condensed table-striped table-hover" id=' +
  this.mainTableId + '>' +
  '  <thead>' +
  '    <tr>' +
  '      <th> </th>' +
  '      <th>ID</th>' +
  '      <th>' + gettext("Name") + '</th>' +
  '      <th>' + gettext("Privilege") + '</th>' +
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
    html =
    '<tr>' +
    '<td><input type="checkbox" class="userRoleSelectCheckbox" ' +
    '           userRoleId="' + role.userRoleId + '"></td>' +
    '<td>' + role.userRoleId + '</td>' +
    '<td>' + role.name + '</td>' +
    '<td>' +
    '<form class="form-inline" style="margin: 0">' +
    '  <input id="editUserRole' + role["userRoleId"] + '"' +
    '    type="button" class="btn"' +
    '    userRoleId="' + role["userRoleId"] + '"' +
    '    value="' + gettext("Show / Edit") + '" />' +
    '</form>' +
    '</td>' +
    '</tr>';
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


var HatoholUserRoleEditor = function(succeededCb, userRole) {
  var self = this;

  self.userRole = userRole;
  self.windowTitle = userRole ?
    gettext("EDIT USER ROLE") : gettext("ADD USER ROLE");
  self.applyButtonTitle = userRole ? gettext("APPLY") : gettext("ADD");

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
    this, ["user-role-editor", self.windowTitle,
           dialogButtons, dialogAttrs]);

  //
  // Dialog button handlers
  //
  function applyButtonClickedCb() {
    if (validateParameters()) {
      makeQueryData();
      if (self.userRole)
        hatoholInfoMsgBox(gettext("Now updating the user ..."));
      else
        hatoholInfoMsgBox(gettext("Now creating a user ..."));
      postAddUserRole();
    }
  }

  function cancelButtonClickedCb() {
    self.closeDialog();
  }

  function makeQueryData() {
      var queryData = {};
      queryData.name = $("#editUserRoleName").val();
      queryData.flags = parseInt($("#editUserRoleFlags").val());
      return queryData;
  }

  function postAddUserRole() {
    var url = "/user-role";
    if (self.userRole)
      url += "/" + self.userRole.userRoleId;
    new HatoholConnector({
      url: url,
      request: self.userRole ? "PUT" : "POST",
      data: makeQueryData(),
      replyCallback: replyCallback,
      parseErrorCallback: hatoholErrorMsgBoxForParser
    });
  }

  function replyCallback(reply, parser) {
    self.closeDialog();
    if (self.userRole)
      hatoholInfoMsgBox(gettext("Successfully updated."));
    else
      hatoholInfoMsgBox(gettext("Successfully created."));

    if (succeededCb)
      succeededCb();
  }

  function validateParameters() {
    if ($("#editUserRoleName").val() == "") {
      hatoholErrorMsgBox(gettext("User role name is empty!"));
      return false;
    }
    if (isNaN($("#editUserRoleFlags").val())) {
      hatoholErrorMsgBox(gettext("Invalid flags!"));
      return false;
    }
    return true;
  }
};

HatoholUserRoleEditor.prototype = Object.create(HatoholDialog.prototype);
HatoholUserRoleEditor.prototype.constructor = HatoholUserRoleEditor;

HatoholUserRoleEditor.prototype.createMainElement = function() {
  var name = self.user ? self.userRole.name : "";
  var flags = self.user ? self.userRole.flags : 0;

  var html =
  '<div>' +
  '<label for="editUserRoleName">' + gettext("User role name") + '</label>' +
  '<input id="editUserRoleName" type="text" value="' + name + '"' +
  '       class="input-xlarge">' +
  // FIXME: Will be replaced with checkboxes
  '<label for="editUserRoleFlags">' + gettext("Flags") + '</label>' +
  '<input id="editUserRoleFlags" type="text" value="' + flags + '"' +
  '       class="input-xlarge">' +
  '</div>';
  return $(html);
};
