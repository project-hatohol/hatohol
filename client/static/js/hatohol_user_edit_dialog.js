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
    alert("Not implemented yet");
    if (succeededCb)
      succeededCb();
  }

  function cancelButtonClickedCb() {
    self.closeDialog();
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
    s += '</div">';
    return s;
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
