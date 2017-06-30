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


var HatoholHostgroupPrivilegeEditDialog = function(userId, serverId, applyCallback) {
  var self = this;
  self.mainTableId = "HostgroupPrivilegeEditDialogMainTable";
  self.userId = userId;
  self.serverId = serverId;
  self.applyCallback = applyCallback;
  self.hostgroupData = null;
  self.loadError = false;

  var dialogButtons = [{
    text: gettext("APPLY"),
    click: function() { self.applyButtonClicked(); }
  }, {
    text: gettext("CANCEL"),
    click: function() { self.cancelButtonClicked(); }
  }];

  // call the constructor of the super class
  var dialogAttrs = { width: "600" };
  HatoholDialog.apply(
    this, ["hostgroup-privilege-edit-dialog", gettext("Edit accessible hostgroups"),
           dialogButtons, dialogAttrs]);
  self.start();
};

HatoholHostgroupPrivilegeEditDialog.prototype =
  Object.create(HatoholDialog.prototype);
HatoholHostgroupPrivilegeEditDialog.prototype.constructor = HatoholHostgroupPrivilegeEditDialog;

HatoholHostgroupPrivilegeEditDialog.prototype.createMainElement = function() {
  var ptag = $("<p/>");
  ptag.attr("id", "hostgroupPrivilegeEditDialogMsgArea");
  ptag.text(gettext("Now getting information..."));
  return ptag;
};

HatoholHostgroupPrivilegeEditDialog.prototype.applyButtonClicked = function() {
  this.applyPrivileges();
  if (this.applyCallback)
    this.applyCallback();
};

HatoholHostgroupPrivilegeEditDialog.prototype.cancelButtonClicked = function() {
  this.closeDialog();
};

HatoholHostgroupPrivilegeEditDialog.prototype.setMessage = function(msg) {
  $("#hostgroupPrivilegeEditDialogMsgArea").text(msg);
};

HatoholHostgroupPrivilegeEditDialog.prototype.start = function() {
  var self = this;
  self.loadError = false;

  loadHostgroups();

  function makeQueryData() {
    var queryData = {};
    queryData.serverId = self.serverId;
    return queryData;
  }

  function loadHostgroups() {
    new HatoholConnector({
      url: "/hostgroup",
      request: "GET",
      data: makeQueryData(),
      replyCallback: function(hostgroupData, parser) {
        if (self.loadError)
          return;
        if (!hostgroupData.numberOfHostgroups) {
          self.setMessage(gettext("No data."));
          return;
        }
        self.hostgroupData = hostgroupData;
        self.updateHostgroupTable();
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

HatoholHostgroupPrivilegeEditDialog.prototype.updateHostgroupTable = function() {
  if (this.loadError)
        return;
  if (!this.hostgroupData)
    return;

  var table = this.generateMainTable();
  var rows = this.generateTableRows(this.serversData);
  this.replaceMainElement(table);
  $("#" + this.mainTableId + " tbody").append(rows);
  this.setupAllCheckButton();
  this.updateAllowCheckboxes();
};

HatoholHostgroupPrivilegeEditDialog.prototype.setupAllCheckButton = function() {
  $(".hostgroupSelectCheckboxes").shiftcheckbox();
  var checkboxes = $(".hostgroupSelectCheckbox");
  $("#all-check-hostgroup").click(function() {
    for (var i = 0; i < checkboxes.length; i++)
      checkboxes[i].checked = true;
  });

  $("#all-uncheck-hostgroup").click(function() {
    for (var i = 0; i < checkboxes.length; i++)
      checkboxes[i].checked = false;
  });
};

HatoholHostgroupPrivilegeEditDialog.prototype.generateMainTable = function() {
  var html =
  '<input type="button" id="all-check-hostgroup" value="' +
  gettext("All check") + '" /> ' +
  '<input type="button" id="all-uncheck-hostgroup" value="' +
  gettext("All uncheck") + '" />' +
  '<table class="table table-condensed table-striped table-hover" id=' +
  this.mainTableId + '>' +
  '  <thead>' +
  '    <tr>' +
  '      <th>' + gettext("Allow") + '</th>' +
  '      <th>' + gettext("Hostgroup ID") + '</th>' +
  '      <th>' + gettext("Hostgroup Name") + '</th>' +
  '    </tr>' +
  '  </thead>' +
  '  <tbody></tbody>' +
  '</table>';
  return html;
};

HatoholHostgroupPrivilegeEditDialog.prototype.generateTableRows = function() {
  var s = '';
  var hostgroups = this.hostgroupData.hostgroups;
  for (var i = 0; i < hostgroups.length; ++i)
  {
    var hostgroup = hostgroups[i];
    s += '<tr>';
    s += '<td class="hostgroupSelectCheckboxes">' +
         '<input type="checkbox" class="hostgroupSelectCheckbox" ' +
               'value="' + hostgroups.indexOf(hostgroup) + '" ' +
               'hostgroupId="' + escapeHTML(hostgroup.groupId) + '"></td>';
    s += '<td>' + escapeHTML(hostgroup.groupId) + '</td>';
    s += '<td>' + escapeHTML(hostgroup.groupName) + '</td>';
    s += '</tr>';
  }
  return s;
};

HatoholHostgroupPrivilegeEditDialog.prototype.updateAllowCheckboxes = function() {
  if (!this.hostgroupData || !this.allowedServers)
    return;

  var checkboxes = $(".hostgroupSelectCheckbox");
  var allowedHostgroup = {};
  if (this.serverId in this.allowedServers)
  {
    allowedHostgroup = this.allowedServers[this.serverId].allowedHostgroups;
  }

  for (var i = 0; i < checkboxes.length; i++)
  {
    hostgroupId = checkboxes[i].getAttribute("hostgroupId");
    if (hostgroupId in allowedHostgroup)
      checkboxes[i].checked = true;
  }
};

HatoholHostgroupPrivilegeEditDialog.prototype.addAccessInfo = function(accessInfo) {
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

HatoholHostgroupPrivilegeEditDialog.prototype.deleteAccessInfo = function(accessInfoId) {
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

HatoholHostgroupPrivilegeEditDialog.prototype.checkApplyResult = function(accessInfo) {
  var result = this.applyResult;
  var numCompleted = result.numSucceeded + result.numFailed;

  hatoholInfoMsgBox(gettext("Aplpying...") + " " +
                    numCompleted + " / " + result.numHostgroups);

  if (numCompleted < result.numHostgroups)
    return;

  // completed
  if (result.numFailed > 0) {
    hatoholErrorMsgBox(gettext("Failed to apply."));
  } else {
    hatoholInfoMsgBox(gettext("Succeeded to apply."));
    this.closeDialog();
  }
};

HatoholHostgroupPrivilegeEditDialog.prototype.applyPrivileges = function() {
  var self = this;
  var checkboxes = $(".hostgroupSelectCheckbox");
  var getAccessInfoId = function(hostgroupId) {
    var allowedHostgroups;

    if (self.serverId in self.allowedServers)
      allowedHostgroups = self.allowedServers[self.serverId].allowedHostgroups;
    else
      return 0;

    var id;
    if (hostgroupId in allowedHostgroups)
    {
      var allowedHostgroup = allowedHostgroups[hostgroupId];
      id = allowedHostgroup.accessInfoId;
    }
    else
    {
      id = 0;
    }
    return id;
  };

  self.applyResult = {
    numHostgroups:   checkboxes.length,
    numSucceeded: 0,
    numFailed:    0
  };
  for (var i = 0; i < checkboxes.length; i++)
  {
    var hostgroupId = checkboxes[i].getAttribute("hostgroupId");
    var accessInfoId = getAccessInfoId(hostgroupId);

    if (checkboxes[i].checked) {
      if (!accessInfoId)
        this.addAccessInfo({ serverId: this.serverId, hostgroupId: hostgroupId });
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
