/*
 * Copyright (C) 2013 Project Hatohol
 *
 * This file is part of Hatohol.
 *
 * Hatohol is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License, version 3
 * as published by the Free Software Foundation.
 *
 * Hatohol is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with Hatohol. If not, see
 * <http://www.gnu.org/licenses/>.
 */

var HatoholLoginDialog = function(readyCallback) {
  var self = this;
  self.readyCallback = readyCallback;

  // We use an own login button that calls submit() of a form tag
  // to make browsers remeber a user name and the password.
  var dialogButtons = [];

  // call the constructor of the super class
  var id = "hatohol_login_dialog";
  var title = gettext("Login");
  var dialogAttr = { width: "auto" };
  HatoholDialog.apply(this, [id, title, dialogButtons, dialogAttr]);
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
    s += '<form method="POST" id="loginForm">';
    s += '<label for="inputUserName">' + gettext("User name") + '</label><br>';
    s += '<input id="inputUserName" type="text" value="" class="input-xlarge"><br>';
    s += '<label for="inputPassword">' + gettext("Password") + '</label><br>';
    s += '<input id="inputPassword" type="password" value="" class="input-xlarge"><br>';
    s += '<div align="center">';
    s += '  <br>';
    s += '  <input type="submit" id="loginFormSubmit" value="' + gettext("Login") + '"/>';
    s += '</div>';
    s += '</form>';
    return s;
  }
}

HatoholLoginDialog.prototype.onAppendMainElement = function () {

  var self = this;
  var validUserName = false;

  $("#inputUserName").keyup(function() {
    validUserName = !!$("#inputUserName").val();
    fixupAddButtonState();
  });

  $("#inputUserName").change(function() {
    validUserName = !!$("#inputUserName").val();
    fixupAddButtonState();
  });

  function fixupAddButtonState() {
    var state = validUserName;
    var btn = $('#loginFormSubmit');
    if (state) {
      btn.removeAttr("disabled");
      btn.removeClass("ui-state-disabled");
    } else {
      btn.attr("disabled", "disable");
      btn.addClass("ui-state-disabled");
    }
  }

  $('#loginForm').submit(function() {
    var user = $("#inputUserName").val();
    var password = $("#inputPassword").val();
    self.readyCallback(user, password);
    return false;
  });
}

//
// Methods defined in this object
//
HatoholLoginDialog.prototype.makeInput = function(user, password) {
  this.readyCallback(user, password);
}
