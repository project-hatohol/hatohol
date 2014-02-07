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

var HatoholUserEditDialog = function(params) {
  var self = this;

  self.operator = params.operator;
  self.user = params.targetUser;
  self.succeededCallback = params.succeededCallback;
  self.userRolesData = null;
  self.windowTitle = self.user ? gettext("EDIT USER") : gettext("ADD USER");
  self.applyButtonTitle = self.user ? gettext("APPLY") : gettext("ADD");

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
    this, ["user-edit-dialog", self.windowTitle, dialogButtons, dialogAttrs]);
  setTimeout(function(){
    self.setApplyButtonState(false);
  }, 1);
  self.loadUserRoles();

  //
  // Dialog button handlers
  //
  function applyButtonClickedCb() {
    if (validateParameters()) {
      makeQueryData();
      if (self.user)
        hatoholInfoMsgBox(gettext("Now updating the user ..."));
      else
        hatoholInfoMsgBox(gettext("Now creating a user ..."));
      postAddUser();
    }
  }

  function cancelButtonClickedCb() {
    self.closeDialog();
  }

  $("#editUserRoles").click(function() {
    new HatoholUserRolesEditor({
      operator: self.operator,
      changedCallback: function() {
        self.loadUserRoles();
      }
    });
  });

  function makeQueryData() {
    var queryData = {};
    var password = $("#editPassword").val();
    queryData.user = $("#editUserName").val();
    if (password)
      queryData.password = password;
    queryData.flags = getFlags();
    return queryData;
  }

  function postAddUser() {
    var url = "/user";
    if (self.user)
      url += "/" + self.user.userId;
    new HatoholConnector({
      url: url,
      request: self.user ? "PUT" : "POST",
      data: makeQueryData(),
      replyCallback: replyCallback,
      parseErrorCallback: hatoholErrorMsgBoxForParser
    });
  }

  function replyCallback(reply, parser) {
    self.closeDialog();
    if (self.user)
      hatoholInfoMsgBox(gettext("Successfully updated."));
    else
      hatoholInfoMsgBox(gettext("Successfully created."));

    if (self.succeededCallback)
      self.succeededCallback();
  }

  function validateParameters() {
    var flags = $("#selectUserRole").val();

    if ($("#editUserName").val() == "") {
      hatoholErrorMsgBox(gettext("User name is empty!"));
      return false;
    }
    if (!self.user && $("#editPassword").val() == "") {
      hatoholErrorMsgBox(gettext("Password is empty!"));
      return false;
    }
    if (isNaN(flags) || flags < 0 || flags > hatohol.ALL_PRIVILEGES) {
      hatoholErrorMsgBox(gettext("Invalid user role!"));
      return false;
    }
    return true;
  }

  function getFlags() {
    var flags = $("#selectUserRole").val();
    return parseInt(flags);
  }
};

HatoholUserEditDialog.prototype = Object.create(HatoholDialog.prototype);
HatoholUserEditDialog.prototype.constructor = HatoholUserEditDialog;

HatoholUserEditDialog.prototype.createMainElement = function() {
  var self = this;
  var div = $(makeMainDivHTML());
  return div;

  function canEditUserRoles() {
    return hasFlag(self.operator, hatohol.OPPRVLG_CREATE_USER_ROLE) ||
      hasFlag(self.operator, hatohol.OPPRVLG_UPDATE_ALL_USER_ROLE) ||
      hasFlag(self.operator, hatohol.OPPRVLG_DELETE_ALL_USER_ROLE);
  };

  function makeMainDivHTML() {
    var userName = self.user ? self.user.name : "";
    var isAdmin = self.user && (self.user.flags == hatohol.ALL_PRIVILEGES);
    var adminSelected = isAdmin ? "selected" : "";
    var html = "" +
    '<div>' +
    '<label for="editUserName">' + gettext("User name") + '</label>' +
    '<input id="editUserName" type="text" value="' + userName +
    '"  class="input-xlarge">' +
    '<label for="editPassword">' + gettext("Password") + '</label>' +
    '<input id="editPassword" type="password" value="" class="input-xlarge">' +
    '<label>' + gettext("User role") + '</label>' +
    '<select id="selectUserRole" style="width: 12em;">' +
    '  <option value="' + hatohol.NONE_PRIVILEGE + '">' +
      gettext('Guest') + '</option>' +
    '  <option value="' + hatohol.ALL_PRIVILEGES + '" ' + adminSelected + '>' +
      gettext('Admin') + '  </option>' +
    '</select>';
    if (canEditUserRoles()) {
      html +=
      '<input id="editUserRoles" type="button" class="btn" ' +
      '  value="' + gettext('EDIT') + '" style="margin-bottom: 10;"/>';
    }
    html += '</div">';
    return html;
  }
};

HatoholUserEditDialog.prototype.onAppendMainElement = function () {
  var self = this;

  $("#editUserName").keyup(function() {
    self.fixupApplyButtonState();
  });

  $("#editPassword").keyup(function() {
    self.fixupApplyButtonState();
  });

  $("#selectUserRole").change(function() {
    fixupApplyButtonState();
  });
};

HatoholUserEditDialog.prototype.setApplyButtonState = function(state) {
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

HatoholUserEditDialog.prototype.fixupApplyButtonState = function() {
  var validUserName = !!$("#editUserName").val();
  var validPassword = !!$("#editPassword").val() || !!this.user;
  var state = (validUserName && validPassword);
  this.setApplyButtonState(state);
};

HatoholUserEditDialog.prototype.updateUserRolesSelector = function() {
  var userRoles = this.userRolesData.userRoles;
  var html = "" +
  '<option value="' + hatohol.NONE_PRIVILEGE + '">' +
    gettext("Guest") + '</option>' +
  '<option value="' + hatohol.ALL_PRIVILEGES + '">' +
    gettext("Admin") + '</option>';

  for (i = 0; i < userRoles.length; i++) {
    html +=
    '<option value="' + userRoles[i].flags + '">' +
    userRoles[i].name +
    '</option>';
  }

  $("#selectUserRole").html(html);
  if (this.user)
    $("#selectUserRole").val(this.user.flags);
};

HatoholUserEditDialog.prototype.loadUserRoles = function() {
  var self = this;
  new HatoholConnector({
    url: "/user-role",
    request: "GET",
    data: {},
    replyCallback: function(userRolesData, parser) {
      self.userRolesData = userRolesData;
      self.updateUserRolesSelector(userRolesData);
    },
    parseErrorCallback: hatoholErrorMsgBoxForParser,
    connectErrorCallback: function(XMLHttpRequest, textStatus, errorThrown) {
      var errorMsg = "Error: " + XMLHttpRequest.status + ": " +
                     XMLHttpRequest.statusText;
      hatoholErrorMsgBox(errorMsg);
    }
  });
};
