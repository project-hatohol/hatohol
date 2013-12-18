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

var HatoholPasswordChanger = function(user, succeededCb) {
  var self = this;

  self.user = user;

  var dialogButtons = [{
    text: gettext("CHANGE"),
    click: changeButtonClickedCb
  }, {
    text: gettext("CANCEL"),
    click: cancelButtonClickedCb
  }];

  // call the constructor of the super class
  dialogAttrs = { width: "auto" };
  HatoholDialog.apply(
    this, ["password-change-dialog", gettext("CHANGE PASSWORD"),
           dialogButtons, dialogAttrs]);
  self.setChangeButtonState(false);

  //
  // Dialog button handlers
  //
  function changeButtonClickedCb() {
    if (validateParameters()) {
      makeQueryData();
      hatoholInfoMsgBox(gettext("Now changing the password ..."));
      postChangePassword();
    }
  }

  function cancelButtonClickedCb() {
    self.closeDialog();
  }

  function makeQueryData() {
      var queryData = {};
      queryData.password = $("#changePassword").val();
      return queryData;
  }

  function postChangePassword() {
    new HatoholConnector({
      url: "/user/" + self.user.userId,
      request: "PUT",
      data: makeQueryData(),
      replyCallback: replyCallback,
      parseErrorCallback: hatoholErrorMsgBoxForParser
    });
  }

  function replyCallback(reply, parser) {
    self.closeDialog();
    hatoholInfoMsgBox(gettext("Successfully changed."));

    if (succeededCb)
      succeededCb();
  }

  function validateParameters() {
    if ($("#changePassword").val() == "") {
      hatoholErrorMsgBox(gettext("Password is empty!"));
      return false;
    }
    if ($("#changePassword").val() != $("#confirmPassword").val()) {
      hatoholErrorMsgBox(gettext("Passwords don't match!"));
      return false;
    }
    return true;
  }
};

HatoholPasswordChanger.prototype = Object.create(HatoholDialog.prototype);
HatoholPasswordChanger.prototype.constructor = HatoholPasswordChanger;

HatoholPasswordChanger.prototype.createMainElement = function() {
  var self = this;
  var div = $(makeMainDivHTML());
  return div;

  function makeMainDivHTML() {
    var s = "";
    s += '<div id="change-password-div">';
    s += '<label for="changePassword">' + gettext("New password") + '</label>';
    s += '<input id="changePassword" type="password" value="" class="input-xlarge">';
    s += '<label for="confirmPassword">' + gettext("New password (Confirm)") + '</label>';
    s += '<input id="confirmPassword" type="password" value="" class="input-xlarge">';
    s += '</div">';
    return s;
  }
};

HatoholPasswordChanger.prototype.onAppendMainElement = function () {
  var self = this;

  function fixupChangeButtonState() {
    var validPassword = !!$("#changePassword").val();
    var matched = $("#changePassword").val() == $("#confirmPassword").val();
    var state = validPassword && matched;
    self.setChangeButtonState(state);
  }

  $("#changePassword").keyup(function() {
    fixupChangeButtonState();
  });

  $("#confirmPassword").keyup(function() {
    fixupChangeButtonState();
  });
};

HatoholPasswordChanger.prototype.setChangeButtonState = function(state) {
  var btn = $(".ui-dialog-buttonpane").find("button:contains(" +
              gettext("CHANGE") + ")");
  if (state) {
     btn.removeAttr("disabled");
     btn.removeClass("ui-state-disabled");
  } else {
     btn.attr("disabled", "disabled");
     btn.addClass("ui-state-disabled");
  }
};
