/*
 * Copyright (C) 2013 Project Hatohol
 *
 * This file is part of Hatohol.
 *
 * Hatohol is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License, version 3
 * as published by the Free Software Foundation.
 *
 * Hatohol is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with Hatohol. If not, see
 * <http://www.gnu.org/licenses/>.
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
    this, ["privilege-edit-dialog", gettext("Edit accessible monitoring servers"),
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
  var self = this;
  const servers = self.serversData.servers;
  const userId = self.userId;
  for (const server of servers)
  {
    const id = "#edit-server" + server.id;
    $(id).click(function() {
      const serverId = this.getAttribute("serverId");
      new HatoholHostgroupPrivilegeEditDialog(
        userId, serverId, applyPrivilegesCb);
    });
  }

  function applyPrivilegesCb() {
    self.start();
  }
};

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
  this.setupAllCheckButton();
  this.updateAllowCheckboxes();
};

HatoholPrivilegeEditDialog.prototype.setupAllCheckButton = function() {
  const checkboxes = $(".serverSelectCheckbox");
  $("#all-check").click(function() {
    for (var i = 0; i < checkboxes.length; i++)
      checkboxes[i].checked = true;
  });

  $("#all-uncheck").click(function() {
    for (var i = 0; i < checkboxes.length; i++)
      checkboxes[i].checked = false;
  });
};

HatoholPrivilegeEditDialog.prototype.generateMainTable = function() {
  var html =
  '<input type="button" id="all-check" value="' +
  gettext("All check") + '" /> ' +
  '<input type="button" id="all-uncheck" value="' +
  gettext("All uncheck") + '" />' +
  '<table class="table table-condensed table-striped table-hover" id=' +
  this.mainTableId + '>' +
  '  <thead>' +
  '    <tr>' +
  '      <th>' + gettext("All Allow") + '</th>' +
  '      <th colspan="2">' + gettext("Accessible Hostgroups") + '</th>' +
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

HatoholPrivilegeEditDialog.prototype.generateTableRows = function() {
  let s = '';
  const servers = this.serversData.servers;
  for (let server of servers)
  {
    s += '<tr>';
    s += '<td><input type="checkbox" class="serverSelectCheckbox" ' +
               'serverId="' + server.id + '"></td>';
    s += '<td>' + escapeHTML(server.numberOfAllowedHostgroups) + '/';
    s +=          escapeHTML(server.numberOfHostgroups) + '</td>';
    s += '<td><input id="edit-server' + server.id + '" type="button"';
    s +=        'serverId="' + escapeHTML(server.id) + '"';
    s +=        'value="' + gettext("Show / Edit") + '" /></td>';
    s += '<td>' + escapeHTML(server.id) + '</td>';
    s += '<td>' + makeMonitoringSystemTypeLabel(server) + '</td>';
    s += '<td>' + escapeHTML(server.hostName) + '</td>';
    s += '<td>' + escapeHTML(server.ipAddress) + '</td>';
    s += '<td>' + escapeHTML(server.nickname) + '</td>';
    s += '</td>';
    s += '</tr>';
  }
  return s;
};

HatoholPrivilegeEditDialog.prototype.updateAllowCheckboxes = function() {
  if (!this.serversData || !this.allowedServers)
    return;

  const checkboxes = $(".serverSelectCheckbox");
  var allHostgroupIsEnabled = function(server) {
    var ALL_HOST_GROUPS = "*", hostgroup;
    if (!server || !server.allowedHostgroups)
      return false;
    hostgroup = server.allowedHostgroups[ALL_HOST_GROUPS];
    return hostgroup && hostgroup.accessInfoId;
  };

  for (let i = 0; i < checkboxes.length; i++)
  {
    const checkbox = checkboxes[i];
    const serverId = checkbox.getAttribute("serverId");
    if (allHostgroupIsEnabled(this.allowedServers[serverId]))
    {
      checkbox.checked = true;
    }
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
  const checkboxes = $(".serverSelectCheckbox");
  var getAccessInfoId = function(serverId) {
    var id, allowedHostgroups, allowedHostgroup;
    var ALL_HOST_GROUPS = "*";
    if (self.allowedServers && self.allowedServers[serverId])
      allowedHostgroups = self.allowedServers[serverId].allowedHostgroups;
    if (allowedHostgroups)
      allowedHostgroup = allowedHostgroups[ALL_HOST_GROUPS];
    if (allowedHostgroup)
      id = allowedHostgroup.accessInfoId;
    return id;
  };

  self.applyResult = {
    numServers:   checkboxes.length,
    numSucceeded: 0,
    numFailed:    0
  };
  for (let i = 0; i < checkboxes.length; i++)
  {
    const checkbox = checkboxes[i];
    const serverId = checkbox.getAttribute("serverId");
    const accessInfoId = getAccessInfoId(serverId);

    if (checkbox.checked) {
      if (!accessInfoId)
        this.addAccessInfo({ serverId: serverId, hostgroupId: "*" });
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
