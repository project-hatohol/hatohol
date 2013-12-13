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

var HatoholPriviledgeEditDialog = function(id, title, selectedCallback) {
  var self = this;
  self.selectedCallback = selectedCallback;
  self.objectArray = null;
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

HatoholPriviledgeEditDialog.prototype = Object.create(HatoholDialog.prototype);
HatoholPriviledgeEditDialog.prototype.constructor = HatoholPriviledgeEditDialog;

HatoholPriviledgeEditDialog.prototype.createMainElement = function() {
  var ptag = $("<p/>");
  ptag.attr("id", "selectorDialogMsgArea");
  ptag.text(gettext("Now getting information..."));
  return ptag;
}

HatoholPriviledgeEditDialog.prototype.selectButtonClicked = function() {
  if (!this.selectedCallback)
    return;
  if (!this.selectedRow)
    this.selectedCallback(null);
  else
    this.selectedCallback(this.objectArray[this.selectedRow.index()]);
  this.closeDialog();
}

HatoholPriviledgeEditDialog.prototype.cancelButtonClicked = function() {
  if (this.selectedCallback)
    this.selectedCallback(null);
  this.closeDialog();
}

HatoholPriviledgeEditDialog.prototype.setSelectButtonState = function(state) {
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

HatoholPriviledgeEditDialog.prototype.setMessage = function(msg) {
  $("#selectorDialogMsgArea").text(msg);
}

HatoholPriviledgeEditDialog.prototype.setObjectArray = function(ary) {
  this.objectArray = ary;
}

HatoholPriviledgeEditDialog.prototype.setSelectedRow = function(row) {
  this.selectedRow = row;
}

HatoholPriviledgeEditDialog.prototype.getSelectedRow = function() {
  return this.selectedRow;
}

HatoholPriviledgeEditDialog.prototype.makeQueryData= function() {
  return {};
}

HatoholPriviledgeEditDialog.prototype.start = function(url, requestType) {
  var self = this;
  $.ajax({
    url: url,
    type: requestType,
    data: this.makeQueryData(),
    success: function(reply) {
      var replyParser = new HatoholReplyParser(reply);
      if (replyParser.getStatus() !== REPLY_STATUS.OK) {
        self.setMessage(replyParser.getStatusMessage());
        return;
      }
      if (self.getNumberOfObjects(reply) == 0) {
        self.setMessage(gettext("No data."));
        return;
      }

      // create a table
      var tableId = "selectorMainTable";
      var table = self.generateMainTable(tableId);
      self.replaceMainElement(table);
      $("#" + tableId + " tbody").append(self.generateTableRows(reply));

      // set events
      $("#" + tableId + " tr").click(function(){
        var selectedRow = self.getSelectedRow();
        if (selectedRow)
          selectedRow.removeClass("info");
        else
          self.setSelectButtonState(true);
        $(this).attr("class", "info");
        self.setSelectedRow($(this));
      });
    },
    error: function(XMLHttpRequest, textStatus, errorThrown) {
      var errorMsg = "Error: " + XMLHttpRequest.status + ": " +
                     XMLHttpRequest.statusText;
      self.setMessage(errorMsg);
    }
  })
}
