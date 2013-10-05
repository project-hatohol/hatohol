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

var HatoholLoginDialog = function(loginCallback) {
  var self = this;
  self.loginCallback = loginCallback;

  var dialogButtons = [{
    text: gettext("Login"),
    click: function() { self.loginButtonClicked() },
  }];

  // call the constructor of the super class
  var id = "hatohol_login_dialog"
  var title = gettext("Login");
  HatoholDialog.apply(this, [id, title, dialogButtons]);
}

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

HatoholLoginDialog.prototype.loginButtonClicked = function() {
  var username = $("#inputUserName").val();
  var password = $("#inputPassword").val();
  this.closeDialog();
  this.loginCallback(username, password);
}
