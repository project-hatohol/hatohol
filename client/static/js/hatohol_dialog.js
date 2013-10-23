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

var HatoholDialogObserver = (function() {
  var createdCallbacks = new Array();

  return {
    reset: function() {
      createdCallbacks.length = 0;
    },

    registerCreatedCallback: function(callback) {
      createdCallbacks.push(callback);
    },

    notifyCreated: function(id, obj) {
      for (var i in createdCallbacks)
        createdCallbacks[i](id, obj);
    }
  }
})();

var HatoholDialog = function(id, dialogTitle, buttons, dialogAttr) {

  var self = this;

  self.mainFrame = $("<div/>");
  self.mainFrame.attr("id", id);
  var elem = self.createMainElement();
  self.mainFrame.append(elem);
  $("body").append(self.mainFrame);
  self.onAppendMainElement();

  var width = "95%";
  if (dialogAttr) {
    if (dialogAttr.width)
      width = dialogAttr.width
  }

  self.dialogId = "#" + id
  $(self.dialogId).dialog({
    autoOpen: false,
    title: dialogTitle,
    closeOnEscape: false,
    width: width,
    modal: true,
    buttons: buttons,
    open: function(event, ui){
      $(".ui-dialog-titlebar-close").hide();
    }
  });

  $(self.dialogId).dialog("open");
  HatoholDialogObserver.notifyCreated(id, this);
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

HatoholDialog.prototype.appendToMainElement = function(elem) {
  this.mainFrame.append(elem);
}

/**
 * Close and delete the dialog.
 */
HatoholDialog.prototype.closeDialog = function() {
  $(this.dialogId).dialog("close");
  $(this.dialogId).remove();
}

HatoholDialog.prototype.setButtonState = function(buttonLabel, state) {
  var btn = $(".ui-dialog-buttonpane").find("button:contains(" +
              buttonLabel + ")");
  if (state) {
     btn.removeAttr("disabled");
     btn.removeClass("ui-state-disabled");
  } else {
     btn.attr("disabled", "disable");
     btn.addClass("ui-state-disabled");
  }
}

