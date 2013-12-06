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

var HatoholUserEditDialog = function(succeededCb) {
  var self = this;

  var dialogButtons = [{
    text: gettext("ADD"),
    click: addButtonClickedCb
  }, {
    text: gettext("CANCEL"),
    click: cancelButtonClickedCb
  }];

  // call the constructor of the super class
  dialogAttrs = { width: "auto" };
  HatoholDialog.apply(
    this, ["user-edit-dialog", gettext("ADD USER"), dialogButtons, dialogAttrs]);
  self.setAddButtonState(false);

  //
  // Dialog button handlers
  //
  function addButtonClickedCb() {
    if (validateParameters()) {
      makeQueryData();
      hatoholInfoMsgBox(gettext("Now creating a user ..."));
      postAddAction();
    }
  }

  function cancelButtonClickedCb() {
    self.closeDialog();
  }

  function makeQueryData() {
      var queryData = {};
      queryData.user = $("#inputUserName").val();
      queryData.password = $("#inputPassword").val();
      queryData.flags = getFlagsFromUserType();
      return queryData;
  }

  function postAddAction() {
    new HatoholConnector({
      url: "/user",
      request: "POST",
      data: makeQueryData(),
      replyCallback: replyCallback,
      parseErrorCallback: hatoholErrorMsgBoxForParser
    });
  }

  function replyCallback(reply, parser) {
    self.closeDialog();
    hatoholInfoMsgBox(gettext("Successfully created."));

    if (succeededCb)
      succeededCb();
  }

  function validateParameters() {
    var type = $("#selectUserType").val();

    if ($("#inputUserName").val() == "") {
      hatoholErrorMsgBox(gettext("User name is empty!"));
      return false;
    }
    if ($("#inputPassword").val() == "") {
      hatoholErrorMsgBox(gettext("Password is empty!"));
      return false;
    }
    if (type != "guest" && type != "admin") {
      hatoholErrorMsgBox(gettext("Invalid user type!"));
      return false;
    }
    return true;
  }

  function getFlagsFromUserType(type) {
    if (!type)
      type = $("#selectUserType").val();
    switch(type) {
    case "admin":
      return hatohol.ALL_PRIVILEGES;
    case "guest":
    default:
      break;
    }
    return hatohol.NONE_PRIVILEGE;
  }
};

HatoholUserEditDialog.prototype = Object.create(HatoholDialog.prototype);
HatoholUserEditDialog.prototype.constructor = HatoholUserEditDialog;

HatoholUserEditDialog.prototype.createMainElement = function() {
  var div = $(makeMainDivHTML());
  return div;

  function makeMainDivHTML() {
    var s = "";
    s += '<div id="add-action-div">';
    s += '<label for="inputUserName">' + gettext("User name") + '</label>';
    s += '<input id="inputUserName" type="text" value="" style="height:1.8em;" class="input-xlarge">';
    s += '<label for="inputPassword">' + gettext("Password") + '</label>';
    s += '<input id="inputPassword" type="password" value="" style="height:1.8em;" class="input-xlarge">';
    s += '<label>' + gettext("User type") + '</label>';
    s += '<select id="selectUserType">';
    s += '  <option value="guest">' + gettext('Guest') + '</option>';
    s += '  <option value="admin">' + gettext('Admin') + '</option>';
    s += '</select>';
    s += '</div">';
    return s;
  }
};

HatoholUserEditDialog.prototype.onAppendMainElement = function () {
  var self = this;
  var validUserName = false;
  var validPassword = false;

  $("#inputUserName").keyup(function() {
    validUserName = !!$("#inputUserName").val();
    fixupAddButtonState();
  });

  $("#inputPassword").keyup(function() {
    validPassword = !!$("#inputPassword").val();
    fixupAddButtonState();
  });

  function fixupAddButtonState() {
    var state = (validUserName && validPassword);
    self.setAddButtonState(state);
  }
};

HatoholUserEditDialog.prototype.setAddButtonState = function(state) {
  var btn = $(".ui-dialog-buttonpane").find("button:contains(" +
              gettext("ADD") + ")");
  if (state) {
     btn.removeAttr("disabled");
     btn.removeClass("ui-state-disabled");
  } else {
     btn.attr("disabled", "disable");
     btn.addClass("ui-state-disabled");
  }
};
