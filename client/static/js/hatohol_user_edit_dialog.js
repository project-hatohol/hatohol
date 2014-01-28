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

var HatoholUserEditDialog = function(succeededCb, user) {
  var self = this;

  self.user = user;
  self.userRolesData = null;
  self.windowTitle = user ? gettext("EDIT USER") : gettext("ADD USER");
  self.applyButtonTitle = user ? gettext("APPLY") : gettext("ADD");

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
  setTimeout(function(){self.setApplyButtonState(false);}, 1);
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
    new HatoholUserRolesEditor();
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

    if (succeededCb)
      succeededCb();
  }

  function validateParameters() {
    var flags = $("#selectUserType").val();

    if ($("#editUserName").val() == "") {
      hatoholErrorMsgBox(gettext("User name is empty!"));
      return false;
    }
    if (!self.user && $("#editPassword").val() == "") {
      hatoholErrorMsgBox(gettext("Password is empty!"));
      return false;
    }
    if (isNaN(flags) || flags < 0 || flags > hatohol.ALL_PRIVILEGES) {
      hatoholErrorMsgBox(gettext("Invalid user type!"));
      return false;
    }
    return true;
  }

  function getFlags() {
    var flags = $("#selectUserType").val();
    return parseInt(flags);
  }
};

HatoholUserEditDialog.prototype = Object.create(HatoholDialog.prototype);
HatoholUserEditDialog.prototype.constructor = HatoholUserEditDialog;

HatoholUserEditDialog.prototype.createMainElement = function() {
  var self = this;
  var div = $(makeMainDivHTML());
  return div;

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
    '<label>' + gettext("User type") + '</label>' +
    '<select id="selectUserType" style="width: 12em;">' +
    '  <option value="guest">' + gettext('Guest') + '</option>' +
    '  <option value="admin" ' + adminSelected + '>' + gettext('Admin') +
    '  </option>' +
    '</select>' +
    '<input id="editUserRoles" type="button" class="btn" ' +
    '  value="' + gettext('EDIT') + '" style="margin-bottom: 10;"/>' +
    '</div">';
    return html;
  }
};

HatoholUserEditDialog.prototype.onAppendMainElement = function () {
  var self = this;
  var validUserName = !!$("#editUserName").val();
  var validPassword = !!$("#editPassword").val() || !!self.user;

  $("#editUserName").keyup(function() {
    validUserName = !!$("#editUserName").val();
    fixupApplyButtonState();
  });

  $("#editPassword").keyup(function() {
    validPassword = !!$("#editPassword").val() || !!self.user;
    fixupApplyButtonState();
  });

  $("#selectUserType").change(function() {
    fixupApplyButtonState();
  });

  function fixupApplyButtonState() {
    var state = (validUserName && validPassword);
    self.setApplyButtonState(state);
  }
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

  $("#selectUserType").html(html);
  if (this.user)
    $("#selectUserType").val(this.user.flags);
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
