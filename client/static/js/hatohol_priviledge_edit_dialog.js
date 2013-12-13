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


var HatoholPriviledgeEditDialog = function(userId, applyCallback) {
  var self = this;
  self.mainTableId = "priviledgeEditDialogMainTable";
  self.userId = userId;
  self.applyCallback = applyCallback;
  self.serversData = null;
  self.priviledgesData = null;
  self.error = false;

  var dialogButtons = [{
    text: gettext("APPLY"),
    click: function() { self.applyButtonClicked(); }
  }, {
    text: gettext("CANCEL"),
    click: function() { self.cancelButtonClicked(); }
  }];

  // call the constructor of the super class
  HatoholDialog.apply(
    this, ["priviledge-edit-dialog", "Edit priviledges", dialogButtons]);
  self.start();
};

HatoholPriviledgeEditDialog.prototype =
  Object.create(HatoholDialog.prototype);
HatoholPriviledgeEditDialog.prototype.constructor = HatoholPriviledgeEditDialog;

HatoholPriviledgeEditDialog.prototype.createMainElement = function() {
  var ptag = $("<p/>");
  ptag.attr("id", "priviledgeEditDialogMsgArea");
  ptag.text(gettext("Now getting information..."));
  return ptag;
};

HatoholPriviledgeEditDialog.prototype.applyButtonClicked = function() {
  this.applyPrivileges();
  if (this.applyCallback)
    this.applyCallback();
  this.closeDialog();
};

HatoholPriviledgeEditDialog.prototype.cancelButtonClicked = function() {
  this.closeDialog();
};

HatoholPriviledgeEditDialog.prototype.setMessage = function(msg) {
  $("#priviledgeEditDialogMsgArea").text(msg);
};

HatoholPriviledgeEditDialog.prototype.start = function() {
  var self = this;
  self.error = false;

  $.ajax({
    url: "/tunnel/server",
    type: "GET",
    data: {},
    success: function(serversData) {
      if (self.error)
        return;
      
      var replyParser = new HatoholReplyParser(serversData);
      if (replyParser.getStatus() !== REPLY_STATUS.OK) {
        self.setMessage(replyParser.getStatusMessage());
        self.erorr = true;
        return;
      }
      if (!serversData.numberOfServers) {
        self.setMessage(gettext("No data."));
        return;
      }

      self.serversData = serversData;
      self.updateServersTable();
    },
    error: function(XMLHttpRequest, textStatus, errorThrown) {
      var errorMsg = "Error: " + XMLHttpRequest.status + ": " +
                     XMLHttpRequest.statusText;
      self.setMessage(errorMsg);
      self.erorr = true;
    }
  });

  var accessInfoURL = "/tunnel/user/" + this.userId + "/access-info";
  $.ajax({
    url: accessInfoURL,
    type: "GET",
    data: {},
    success: function(priviledgesData) {
      if (self.error)
        return;
      
      var replyParser = new HatoholReplyParser(priviledgesData);
      if (replyParser.getStatus() !== REPLY_STATUS.OK) {
        self.setMessage(replyParser.getStatusMessage());
        self.erorr = true;
        return;
      }

      self.priviledgesData = priviledgesData;
      self.updateAllowCheckboxes();
    },
    error: function(XMLHttpRequest, textStatus, errorThrown) {
      var errorMsg = "Error: " + XMLHttpRequest.status + ": " +
                     XMLHttpRequest.statusText;
      self.setMessage(errorMsg);
      self.erorr = true;
    }
  });
};

HatoholPriviledgeEditDialog.prototype.updateServersTable = function() {
  if (this.error)
        return;
  if (!this.serversData)
    return;

  var table = this.generateMainTable();
  var rows = this.generateTableRows(this.serversData);
  this.replaceMainElement(table);
  $("#" + this.mainTableId + " tbody").append(rows);
  this.updateAllowCheckboxes();
};

HatoholPriviledgeEditDialog.prototype.generateMainTable = function() {
  var html =
  '<table class="table table-condensed table-striped table-hover" id=' +
  this.mainTableId + '>' +
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

HatoholPriviledgeEditDialog.prototype.generateTableRows = function() {
  var s = '';
  var servers = this.serversData.servers;
  for (var i = 0; i < servers.length; i++) {
    sv = servers[i];
    s += '<tr>';
    s += '<td><input type="checkbox" class="serverSelectCheckbox" ' +
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

HatoholPriviledgeEditDialog.prototype.updateAllowCheckboxes = function() {
  if (!this.serversData || !this.priviledgesData)
    return;
  // FIXME
};

HatoholPriviledgeEditDialog.prototype.applyPrivileges = function() {
  alert("Not implemented yet.");
  var i, serverId, checkboxes = $(".serverSelectCheckbox");
  for (i = 0; i < checkboxes.length; i++) {
    serverId = checkboxes[i].getAttribute("serverId");

    // FIXME: check whether the checkbox is changed or not

    if (checkboxes[i].checked) {
      // FIXME: add accessInfo
    } else {
      // FIXME: delete accessInfo
    }
  }
};
