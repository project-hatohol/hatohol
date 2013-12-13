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


var HatoholPriviledgeSelector = function(applyCallback) {
  var self = this;
  self.applyCallback = applyCallback;
  self.objectArray = null;

  var dialogButtons = [{
    text: gettext("APPLY"),
    click: function() { self.applyButtonClicked(); }
  }, {
    text: gettext("CANCEL"),
    click: function() { self.cancelButtonClicked(); }
  }];

  // call the constructor of the super class
  HatoholDialog.apply(
    this, ["priviledge-selector", "Edit priviledges", dialogButtons]);
  self.start();
};

HatoholPriviledgeSelector.prototype =
  Object.create(HatoholSelectorDialog.prototype);
HatoholPriviledgeSelector.prototype.constructor = HatoholPriviledgeSelector;

HatoholPriviledgeSelector.prototype.createMainElement = function() {
  var ptag = $("<p/>");
  ptag.attr("id", "priviledgeSelectorDialogMsgArea");
  ptag.text(gettext("Now getting information..."));
  return ptag;
};

HatoholPriviledgeSelector.prototype.applyButtonClicked = function() {
  if (!this.applyCallback)
    return;
  this.applyCallback();
  this.closeDialog();
};

HatoholPriviledgeSelector.prototype.cancelButtonClicked = function() {
  this.closeDialog();
};

HatoholPriviledgeSelector.prototype.setMessage = function(msg) {
  $("#selectorDialogMsgArea").text(msg);
};

HatoholPriviledgeSelector.prototype.setObjectArray = function(ary) {
  this.objectArray = ary;
};

HatoholPriviledgeSelector.prototype.makeQueryData= function() {
  return {};
};

HatoholPriviledgeSelector.prototype.start = function() {
  var self = this;
  $.ajax({
    url: "/tunnel/server",
    type: "GET",
    data: this.makeQueryData(),
    success: function(reply) {
      var replyParser = new HatoholReplyParser(reply);
      if (replyParser.getStatus() !== REPLY_STATUS.OK) {
        self.setMessage(replyParser.getStatusMessage());
        return;
      }
      if (!reply.numberOfServers) {
        self.setMessage(gettext("No data."));
        return;
      }

      // create a table
      var tableId = "selectorMainTable";
      var table = self.generateMainTable(tableId);
      self.replaceMainElement(table);
      $("#" + tableId + " tbody").append(self.generateTableRows(reply));
    },
    error: function(XMLHttpRequest, textStatus, errorThrown) {
      var errorMsg = "Error: " + XMLHttpRequest.status + ": " +
                     XMLHttpRequest.statusText;
      self.setMessage(errorMsg);
    }
  });
};

HatoholPriviledgeSelector.prototype.generateMainTable = function(tableId) {
  var html =
  '<table class="table table-condensed table-striped table-hover" id=' +
  tableId + '>' +
  '  <thead>' +
  '    <tr>' +
  '      <th>' + gettext("Allow") + '</th>' +
  '      <th>ID</th>' +
  '      <th>' + gettext("Type") + '</th>' +
  '      <th>' + gettext("Hostname") + '</th>' +
  '      <th>' + gettext("IP Address") + '</th>' +
  '      <th>' + gettext("Nickname") + '</th>' +
  '    </tr>' +
  '  </thead>' +
  '  <tbody></tbody>' +
  '</table>';
  return html;
};

HatoholPriviledgeSelector.prototype.generateTableRows = function(reply) {
  var s = '';
  this.setObjectArray(reply.servers);
  for (var i = 0; i < reply.servers.length; i++) {
    sv = reply.servers[i];
    s += '<tr>';
    s += '<td><input type="checkbox" class="selectcheckbox" ' +
               'serverId="' + sv['id'] + '"></td>';
    s += '<td>' + sv.id + '</td>';
    s += '<td>' + makeMonitoringSystemTypeLabel(sv.type) + '</td>';
    s += '<td>' + sv.hostName + '</td>';
    s += '<td>' + sv.ipAddress + '</td>';
    s += '<td>' + sv.nickname  + '</td>';
    s += '</tr>';
  }
  return s;
};
