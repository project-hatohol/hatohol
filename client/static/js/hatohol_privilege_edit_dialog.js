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


var HatoholPrivilegeEditDialog = function(userId, applyCallback) {
  var self = this;
  self.mainTableId = "privilegeEditDialogMainTable";
  self.userId = userId;
  self.applyCallback = applyCallback;
  self.serversData = null;
  self.loadError = false;

  var dialogButtons = [{
    text: gettext("APPLY"),
    click: function() { self.applyButtonClicked(); }
  }, {
    text: gettext("CANCEL"),
    click: function() { self.cancelButtonClicked(); }
  }];

  // call the constructor of the super class
  var dialogAttrs = { width: "800" };
  HatoholDialog.apply(
    this, ["privilege-edit-dialog", gettext("Edit privileges"),
           dialogButtons, dialogAttrs]);
  self.start();
};

HatoholPrivilegeEditDialog.prototype =
  Object.create(HatoholDialog.prototype);
HatoholPrivilegeEditDialog.prototype.constructor = HatoholPrivilegeEditDialog;

HatoholPrivilegeEditDialog.prototype.createMainElement = function() {
  var ptag = $("<p/>");
  ptag.attr("id", "privilegeEditDialogMsgArea");
  ptag.text(gettext("Now getting information..."));
  return ptag;
};

HatoholPrivilegeEditDialog.prototype.applyButtonClicked = function() {
  this.applyPrivileges();
  if (this.applyCallback)
    this.applyCallback();
};

HatoholPrivilegeEditDialog.prototype.cancelButtonClicked = function() {
  this.closeDialog();
};

HatoholPrivilegeEditDialog.prototype.setMessage = function(msg) {
  $("#privilegeEditDialogMsgArea").text(msg);
};

HatoholPrivilegeEditDialog.prototype.setupSelectHostgroupDialog = function() {
  var servers = this.serversData.servers;
  var id;
  for (var i = 0; i < servers.length; i++) {
    id = "#edit-server" + servers[i]["id"];
    $(id).click(function() {
      var serverId = this.getAttribute("serverId");
      new HatoholHostgroupPrivilegeEditDialog(this.userId, serverId);
    });
  }
}

HatoholPrivilegeEditDialog.prototype.start = function() {
  var self = this;
  self.loadError = false;

  loadServers();

  function makeQueryData() {
    var queryData = {};
    queryData.showHostgroup = 1;
    queryData.targetUser = self.userId;
    return queryData;
  }

  function loadServers() {
    new HatoholConnector({
      url: "/server",
      request: "GET",
      data: makeQueryData(),
      replyCallback: function(serversData, parser) {
        if (self.loadError)
          return;
        if (!serversData.numberOfServers) {
          self.setMessage(gettext("No data."));
          return;
        }
        self.serversData = serversData;
        self.updateServersTable();
        self.setupSelectHostgroupDialog();
        loadAccessInfo();
      },
      parseErrorCallback: function(reply, parser) {
        if (self.loadError)
          return;
        self.setMessage(parser.getMessage());
        self.loadError = true;
      },
      connectErrorCallback: function(XMLHttpRequest, textStatus, errorThrown) {
        var errorMsg = "Error: " + XMLHttpRequest.status + ": " +
                       XMLHttpRequest.statusText;
        self.setMessage(errorMsg);
        self.erorr = true;
      }
    });
  }

  function loadAccessInfo() {
    new HatoholConnector({
      url: "/user/" + self.userId + "/access-info",
      request: "GET",
      data: {},
      replyCallback: function(reply, parser) {
        if (self.loadError)
          return;
        self.allowedServers = reply.allowedServers;
        self.updateAllowCheckboxes();
      },
      parseErrorCallback: function(reply, parser) {
        self.setMessage(parser.getMessage());
        self.loadError = true;
      },
      connectErrorCallback: function(XMLHttpRequest, textStatus, errorThrown) {
        var errorMsg = "Error: " + XMLHttpRequest.status + ": " +
                       XMLHttpRequest.statusText;
        self.setMessage(errorMsg);
        self.erorr = true;
      }
    });
  }
};

HatoholPrivilegeEditDialog.prototype.updateServersTable = function() {
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

HatoholPrivilegeEditDialog.prototype.generateMainTable = function() {
  var html =
  '<table class="table table-condensed table-striped table-hover" id=' +
  this.mainTableId + '>' +
  '  <thead>' +
  '    <tr>' +
  '      <th>' + gettext("Allow") + '</th>' +
  '      <th>' + gettext("Allowed Hostgroups") + '</th>' +
  '      <th>ID</th>' +
  '      <th>' + gettext("Type") + '</th>' +
  '      <th>' + gettext("Hostname") + '</th>' +
  '      <th>' + gettext("IP Address") + '</th>' +
  '      <th>' + gettext("Nickname") + '</th>' +
  '      <th>' + gettext("Hostgroup") + '</th>' +
  '    </tr>' +
  '  </thead>' +
  '  <tbody></tbody>' +
  '</table>';
  return html;
};

HatoholPrivilegeEditDialog.prototype.generateTableRows = function() {
  var s = '';
  var servers = this.serversData.servers;
  for (var i = 0; i < servers.length; i++) {
    sv = servers[i];
    s += '<tr>';
    s += '<td><input type="checkbox" class="serverSelectCheckbox" ' +
               'serverId="' + sv['id'] + '"></td>';
    s += '<td>' + escapeHTML(sv.numberOfAllowedHostgroups) + '/';
    s +=          escapeHTML(sv.numberOfHostgroups) + '</td>';
    s += '<td>' + escapeHTML(sv.id) + '</td>';
    s += '<td>' + makeMonitoringSystemTypeLabel(sv.type) + '</td>';
    s += '<td>' + escapeHTML(sv.hostName) + '</td>';
    s += '<td>' + escapeHTML(sv.ipAddress) + '</td>';
    s += '<td>' + escapeHTML(sv.nickname)  + '</td>';
    s += '<td><input id="edit-server' + sv['id'] + '" type="button"' +
              'serverId="' + escapeHTML(sv.id) + '"' +
              'value="' + gettext("Show / Edit") + '" />';
    s += '</td>';
    s += '</tr>';
  }
  return s;
};

HatoholPrivilegeEditDialog.prototype.updateAllowCheckboxes = function() {
  if (!this.serversData || !this.allowedServers)
    return;

  var i, serverId, checkboxes = $(".serverSelectCheckbox");
  var allHostGroupIsEnabled = function(server) {
    var ALL_HOST_GROUPS = -1, hostGroup;
    if (!server || !server["allowedHostGroups"])
      return false;
    hostGroup = server["allowedHostGroups"][ALL_HOST_GROUPS];
    return hostGroup && hostGroup["accessInfoId"];
  };

  for (i = 0; i < checkboxes.length; i++) {
    serverId = checkboxes[i].getAttribute("serverId");
    if (allHostGroupIsEnabled(this.allowedServers[serverId]))
      checkboxes[i].checked = true;
  }
};

HatoholPrivilegeEditDialog.prototype.addAccessInfo = function(accessInfo) {
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

HatoholPrivilegeEditDialog.prototype.deleteAccessInfo = function(accessInfoId) {
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

HatoholPrivilegeEditDialog.prototype.checkApplyResult = function(accessInfo) {
  var result = this.applyResult;
  var numCompleted = result.numSucceeded + result.numFailed;

  hatoholInfoMsgBox(gettext("Aplpying...") + " " +
                    numCompleted + " / " + result.numServers);

  if (numCompleted < result.numServers)
    return;

  // completed
  if (result.numFailed > 0) {
    hatoholErrorMsgBox(gettext("Failed to apply."));
  } else {
    hatoholInfoMsgBox(gettext("Succeeded to apply."));
    this.closeDialog();
  }
};

HatoholPrivilegeEditDialog.prototype.applyPrivileges = function() {
  var self = this;
  var i, serverId, accessInfoId;
  var checkboxes = $(".serverSelectCheckbox");
  var getAccessInfoId = function(serverId) {
    var id, allowedHostGroups, allowedHostGroup;
    var ALL_HOST_GROUPS = -1;
    if (self.allowedServers && self.allowedServers[serverId])
      allowedHostGroups = self.allowedServers[serverId]["allowedHostGroups"];
    if (allowedHostGroups)
      allowedHostGroup = allowedHostGroups[ALL_HOST_GROUPS];
    if (allowedHostGroup)
      id = allowedHostGroup["accessInfoId"];
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
