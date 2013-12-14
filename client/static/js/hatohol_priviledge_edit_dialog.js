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
  self.loadError = false;

  var dialogButtons = [{
    text: gettext("APPLY"),
    click: function() { self.applyButtonClicked(); }
  }, {
    text: gettext("CANCEL"),
    click: function() { self.cancelButtonClicked(); }
  }];

  // call the constructor of the super class
  HatoholDialog.apply(
    this, ["priviledge-edit-dialog", gettext("Edit priviledges"),
           dialogButtons]);
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
};

HatoholPriviledgeEditDialog.prototype.cancelButtonClicked = function() {
  this.closeDialog();
};

HatoholPriviledgeEditDialog.prototype.setMessage = function(msg) {
  $("#priviledgeEditDialogMsgArea").text(msg);
};

HatoholPriviledgeEditDialog.prototype.start = function() {
  var self = this;
  self.loadError = false;

  new HatoholConnector({
    url: "/server",
    request: "GET",
    data: {},
    replyCallback: function(serversData, parser) {
      if (self.loadError)
        return;
      if (!serversData.numberOfServers) {
        self.setMessage(gettext("No data."));
        return;
      }
      self.serversData = serversData;
      self.updateServersTable();
    },
    parseErrorCallback: function(reply, parser) {
      if (self.loadError)
        return;
      self.setMessage(parser.getStatusMessage());
      self.loadError = true;
    },
    connectErrorCallback: function(XMLHttpRequest, textStatus, errorThrown) {
      var errorMsg = "Error: " + XMLHttpRequest.status + ": " +
                     XMLHttpRequest.statusText;
      self.setMessage(errorMsg);
      self.erorr = true;
    }
  });

  new HatoholConnector({
    url: "/user/" + self.userId + "/access-info",
    request: "GET",
    data: {},
    replyCallback: function(priviledgesData, parser) {
      if (self.loadError)
        return;
      self.allowedServers = self.parsePriviledgesData(priviledgesData);
      self.priviledgesData = priviledgesData;
      self.updateAllowCheckboxes();
    },
    parseErrorCallback: function(reply, parser) {
      self.setMessage(parser.getStatusMessage());
      self.loadError = true;
    },
    connectErrorCallback: function(XMLHttpRequest, textStatus, errorThrown) {
      var errorMsg = "Error: " + XMLHttpRequest.status + ": " +
                     XMLHttpRequest.statusText;
      self.setMessage(errorMsg);
      self.erorr = true;
    }
  });
};

HatoholPriviledgeEditDialog.prototype.updateServersTable = function() {
  if (this.loadError)
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

  var i, serverId, checkboxes = $(".serverSelectCheckbox");
  var allHostGroupIsEnabled = function(server) {
    return server && server[-1];
  };

  for (i = 0; i < checkboxes.length; i++) {
    serverId = checkboxes[i].getAttribute("serverId");
    if (allHostGroupIsEnabled(this.allowedServers[serverId]))
      checkboxes[i].checked = true;
  }
};

HatoholPriviledgeEditDialog.prototype.parsePriviledgesData = function(data) {
  var i, serversTable = {}, hostGroupsTable;
  var allowedServers = data.allowedServers, allowedHostGroups;
  var serverId, hostGroupId;
  for (i = 0; i < allowedServers.length; i++) {
    serverId = allowedServers[i]["serverId"];
    allowedHostGroups = allowedServers[i]["allowedHostGroups"];
    hostGroupsTable = {};
    for (j = 0; j < allowedHostGroups.length; j++) {
      hostGroupId = allowedHostGroups[j]["hostGroupId"];
      hostGroupsTable[hostGroupId] = allowedHostGroups[j]["accessInfoId"];
    }
    serversTable[serverId] = hostGroupsTable;
  }
  return serversTable;
};

HatoholPriviledgeEditDialog.prototype.addAccessInfo = function(accessInfo) {
  var self = this;
  var userId = this.userId;
  new HatoholConnector({
    url: "/user/" + userId + "/access-info",
    request: "POST",
    data: accessInfo,
    replyCallback: function(reply, parser) {
      self.applyResult.numSucceeded += 1;
      self.checkApplyResult();
    },
    parseErrorCallback: function(reply, parser) {
      self.applyResult.numFailed += 1;
      self.checkApplyResult();
    },
    connectErrorCallback: function(XMLHttpRequest, textStatus, errorThrown) {
      self.applyResult.numFailed += 1;
      self.checkApplyResult();
    }
  });
};

HatoholPriviledgeEditDialog.prototype.deleteAccessInfo = function(accessInfoId) {
  var self = this;
  var userId = this.userId;
  new HatoholConnector({
    url: "/user/" + userId + "/access-info/" + accessInfoId,
    request: "DELETE",
    replyCallback: function(reply, parser) {
      self.applyResult.numSucceeded += 1;
      self.checkApplyResult();
    },
    parseErrorCallback: function(reply, parser) {
      self.applyResult.numFailed += 1;
      self.checkApplyResult();
    },
    connectErrorCallback: function(XMLHttpRequest, textStatus, errorThrown) {
      self.applyResult.numFailed += 1;
      self.checkApplyResult();
    }
  });
};

HatoholPriviledgeEditDialog.prototype.checkApplyResult = function(accessInfo) {
  var result = this.applyResult;
  var numCompleted = result.numSucceeded + result.numFailed;
  if (numCompleted < result.numServers)
    return;
  if (result.numFailed > 0)
    hatoholInfoMsgBox(gettext("Failed to apply."));
  else
    hatoholInfoMsgBox(gettext("Succeeded to apply."));
  this.closeDialog();
};

HatoholPriviledgeEditDialog.prototype.applyPrivileges = function() {
  var self = this;
  var i, serverId, accessInfoId;
  var checkboxes = $(".serverSelectCheckbox");
  var getAccessInfoId = function(serverId) {
    var id;
    if (self.allowedServers && self.allowedServers[serverId])
      id = self.allowedServers[serverId][-1];
    return id;
  };

  self.applyResult = {
    numServers:   checkboxes.length,
    numSucceeded: 0,
    numFailed:    0
  };
  for (i = 0; i < checkboxes.length; i++) {
    serverId = checkboxes[i].getAttribute("serverId");
    accessInfoId = getAccessInfoId(serverId);

    if (checkboxes[i].checked) {
      if (!accessInfoId)
        this.addAccessInfo({ serverId: serverId, hostGroupId: -1 });
      else
        self.applyResult.numSucceeded += 1;
    } else {
      if (accessInfoId)
        this.deleteAccessInfo(accessInfoId);
      else
        self.applyResult.numSucceeded += 1;
    }
  }
  self.checkApplyResult();
};
