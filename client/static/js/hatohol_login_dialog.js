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

var HatoholLoginDialog = function(loginCallback, tryLoginCallback) {
  var self = this;
  self.loginCallback = loginCallback;
  self.buttonName = gettext("Login");

  var dialogButtons = [{
    text: self.buttonName,
    click: function() { loginButtonClicked(); },
  }];

  // call the constructor of the super class
  var id = "hatohol_login_dialog";
  var title = gettext("Login");
  dialogAttr = {};
  dialogAttr.width = "auto";
  HatoholDialog.apply(this, [id, title, dialogButtons, dialogAttr]);
  self.setButtonState(self.buttonName, false);

  function loginButtonClicked() {
    var user = $("#inputUserName").val();
    var password = $("#inputPassword").val();
    if (tryLoginCallback)
      tryLoginCallback(user, password);

    $.ajax({
      url: "/tunnel/login&user=" + encodeURI(user) + "&password=" + encodeURI(password),
      type: "GET",
      success: function(data) {
        parseLoginResult(data);
      },
      error: function(XMLHttpRequest, textStatus, errorThrown) {
        var errorMsg = "Error: " + XMLHttpRequest.status + ": " +
                       XMLHttpRequest.statusText;
        hatoholErrorMsgBox(errorMsg);
      },
    });
  }

  function parseLoginResult(data) {
    var parser = new HatoholLoginReplyParser(data);
    if (parser.getStatus() != REPLY_STATUS.OK) {
      var msg = gettext("Failed to login.") + parser.getStatusMessage();
      hatoholErrorMsgBox(msg);
      return;
    }
    sessionId = parser.getSessionId();
    self.closeDialog();
    self.loginCallback(sessionId);
  }
}

//
// Override methods
//
HatoholLoginDialog.prototype = Object.create(HatoholDialog.prototype);
HatoholLoginDialog.prototype.constructor = HatoholLoginDialog;

HatoholLoginDialog.prototype.createMainElement = function() {
  var div = $(makeMainDivHTML());
  return div;

  function makeMainDivHTML(initParams) {
    var s = '';
    s += '<label for="inputUserName">' + gettext("User name") + '</label>';
    s += '<input id="inputUserName" type="text" value="" style="height:1.8em;" class="input-xlarge">';
    s += '<label for="inputPassword">' + gettext("Password") + '</label>';
    s += '<input id="inputPassword" type="password" value="" style="height:1.8em;" class="input-xlarge">';
    return s;
  }
}

HatoholLoginDialog.prototype.onAppendMainElement = function () {

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
    self.setButtonState(self.buttonName, state);
  }
}

//
// Methods defined in this object
//
