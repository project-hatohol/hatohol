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
};

HatoholUserRolesEditor.prototype = Object.create(HatoholDialog.prototype);
HatoholUserRolesEditor.prototype.constructor = HatoholUserRolesEditor;

HatoholUserRolesEditor.prototype.createMainElement = function() {
  var self = this;
  var div = $(makeMainDivHTML());
  return div;

  function makeMainDivHTML() {
    var html = "";
    html += '<div></div>';
    return html;
  }
};

HatoholUserRolesEditor.prototype.onAppendMainElement = function () {
};
