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

var HatoholDialog = function(id, dialogTitle, buttons) {

  var self = this;

  self.mainFrame = $("<div/>");
  self.mainFrame.attr("id", id);
  var elem = self.createMainElement();
  self.mainFrame.append(elem);
  $("body").append(self.mainFrame);
  self.onAppendMainElement();

  self.dialogId = "#" + id
  $(self.dialogId).dialog({
    autoOpen: false,
    title: dialogTitle,
    closeOnEscape: false,
    width:  "95%",
    modal: true,
    buttons: buttons,
    open: function(event, ui){
      $(".ui-dialog-titlebar-close").hide();
    }
  });

  $(self.dialogId).dialog("open");
}

/**
 * Called just after the main element is appended to the HTML body.
 */
HatoholDialog.prototype.onAppendMainElement = function () {
}

/**
 * Replace a main element of the dialog.
 * Note that the old element is deleted.
 *
 * @param elem A new element to be set.
 */
HatoholDialog.prototype.replaceMainElement = function(elem) {
  this.mainFrame.empty();
  this.mainFrame.append(elem);
}

/**
 * Close and delete the dialog.
 */
HatoholDialog.prototype.closeDialog = function() {
  $(this.dialogId).dialog("close");
  $(this.dialogId).remove();
}
