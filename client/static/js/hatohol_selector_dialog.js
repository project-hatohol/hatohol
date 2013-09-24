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

var HatoholSelectorDialog = function(id, title, selectedCallback) {
  var self = this;
  self.selectedCallback = selectedCallback;
  self.selectedRow = null;

  var dialogButtons = [{
    text: gettext("SELECT"),
    click: function() { self.selectButtonClicked() },
  }, {
    text: gettext("CANCEL"),
    click: function() { self.cancelButtonClicked() },
  }];

  // call the constructor of the super class
  HatoholDialog.apply(this, [id, title, dialogButtons]);
  self.setSelectButtonState(false);
}

HatoholSelectorDialog.prototype = Object.create(HatoholDialog.prototype);
HatoholSelectorDialog.prototype.constructor = HatoholSelectorDialog;

HatoholSelectorDialog.prototype.createMainElement = function() {
  var ptag = $("<p/>");
  ptag.attr("id", "selectorDialogMsgArea");
  ptag.text(gettext("Now getting information..."));
  return ptag;
}

HatoholSelectorDialog.prototype.selectButtonClicked = function() {
  if (!this.selectedCallback)
    return;
  if (!this.selectedRow)
    this.selectedCallback(null);
  else
    this.selectedCallback(this.serverArray[this.selectedRow.index()]);
  this.closeDialog();
}

HatoholSelectorDialog.prototype.cancelButtonClicked = function() {
  if (this.selectedCallback)
    this.selectedCallback(null);
  this.closeDialog();
}

HatoholSelectorDialog.prototype.setSelectButtonState = function(state) {
  var btn = $(".ui-dialog-buttonpane").find("button:contains(" +
              gettext("SELECT") + ")");
  if (state) {
     btn.removeAttr("disabled");
     btn.removeClass("ui-state-disabled");
  } else {
     btn.attr("disabled", "disable");
     btn.addClass("ui-state-disabled");
  }
}

HatoholSelectorDialog.prototype.setMessage = function(msg) {
  $("#selectorDialogMsgArea").text(msg);
}
