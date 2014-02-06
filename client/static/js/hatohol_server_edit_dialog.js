/*
 * Copyright (C) 2013-2014 Project Hatohol
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

var HatoholServerEditDialog = function(params) {
  var self = this;

  self.operator = params.operator;
  self.server = params.targetServer;
  self.succeededCallback = params.succeededCallback;
  self.windowTitle =
    self.server ? gettext("EDIT SERVER") : gettext("ADD SERVER");
  self.applyButtonTitle = self.server ? gettext("APPLY") : gettext("ADD");

  var dialogButtons = [{
    text: self.applyButtonTitle,
    click: addButtonClickedCb
  }, {
    text: gettext("CANCEL"),
    click: cancelButtonClickedCb
  }];

  // call the constructor of the super class
  dialogAttrs = { width: "auto" };
  HatoholDialog.apply(
      this, ["server-edit-dialog", self.windowTitle,
             dialogButtons, dialogAttrs]);
  if (self.server)
    self.setServer(self.server);
  setTimeout(function(){
    self.fixupApplyButtonState();
  }, 1);

  //
  // Dialog button handlers
  //
  function addButtonClickedCb() {
    if (validateParameters()) {
      makeQueryData();
      hatoholInfoMsgBox(gettext("Now adding a server..."));
      postAddServer();
    }
  }

  function cancelButtonClickedCb() {
    self.closeDialog();
  }

  function makeQueryData() {
    var queryData = {};
    queryData.type = $("#selectServerType").val();
    queryData.hostName = $("#inputHostName").val();
    queryData.ipAddress = $("#inputIpAddress").val();
    queryData.nickname = $("#inputNickName").val();
    queryData.port = $("#inputPort").val();
    queryData.pollingInterval = $("#inputPollingInterval").val();
    queryData.retryInterval = $("#inputRetryInterval").val();
    queryData.user = $("#inputUserName").val();
    queryData.password = $("#inputPassword").val();
    queryData.dbName = $("#inputDbName").val();
    return queryData;
  }

  function postAddServer() {
    var url = "/server";
    if (self.server)
      url += "/" + self.server.id;
    new HatoholConnector({
      url: url,
      request: self.server ? "PUT" : "POST",
      data: makeQueryData(),
      replyCallback: replyCallback,
      parseErrorCallback: hatoholErrorMsgBoxForParser
    });
  }
  
  function replyCallback(reply, parser) {
    self.closeDialog();
    hatoholInfoMsgBox(gettext("Successfully created."));

    if (self.succeededCallback)
      self.succeededCallback();
  }

  function validateParameters() {
    var type = $("#selectServerType").val();

    if (type != hatohol.MONITORING_SYSTEM_ZABBIX &&
        type != hatohol.MONITORING_SYSTEM_NAGIOS)
    {
      hatoholErrorMsgBox(gettext("Invalid Server type!"));
      return false;
    }
    if ($("#inputHostName").val() == "") {
      hatoholErrorMsgBox(gettext("Host name is empty!"));
      return false;
    }
    if ($("#inputIpAddress").val() == "") {
      hatoholErrorMsgBox(gettext("IP address is empty!"));
      return false;
    }
    if ($("#inputNickName").val() == "") {
      hatoholErrorMsgBox(gettext("NickName is empty!"));
      return false;
    }
    if ($("#inputPort").val() == "") {
      hatoholErrorMsgBox(gettext("Port is empty!"));
      return false;
    }
    if ($("#inputPollingInterval").val() == "") {
      hatoholErrorMsgBox(gettext("Polling interval is empty!"));
      return false;
    }
    if ($("#inputRetryInterval").val() == "") {
      hatoholErrorMsgBox(gettext("Retry interval is empty!"));
      return false;
    }
    if ($("#inputUserName").val() == "") {
      hatoholErrorMsgBox(gettext("User Name is empty!"));
      return false;
    }
    if ($("#inputPassWord").val() == "") {
      hatoholErrorMsgBox(gettext("Password is empty!"));
      return false;
    }
    if (type == hatohol.MONITORING_SYSTEM_NAGIOS &&
        $("#inputDbName").val() == "")
    {
      hatoholErrorMsgBox(gettext("DB name is empty!"));
      return false;
    }
    return true;
  }
};

HatoholServerEditDialog.prototype = Object.create(HatoholDialog.prototype);
HatoholServerEditDialog.prototype.constructor = HatoholServerEditDialog;

HatoholServerEditDialog.prototype.createMainElement = function() {
  var div = $(makeMainDivHTML());
  return div;

  function makeMainDivHTML() {
    var s = "";
    s += '<div id="add-action-div">';
    s += '<form class="form-inline">';
    s += '  <label>' + gettext("Server type") + '</label>';
    s += '  <select id="selectServerType" style="width:10em">';
    s += '    <option value="' + hatohol.MONITORING_SYSTEM_ZABBIX + '">' +
      gettext("Zabbix") + '</option>';
    s += '    <option value="' + hatohol.MONITORING_SYSTEM_NAGIOS +'">' +
      gettext("Nagios") + '</option>';
    s += '  </select>';
    s += '</form>';
    s += '<form class="form-inline">';
    s += '  <label for="inputNickName">' + gettext("Nickname") + '</label>';
    s += '  <input id="inputNickName" type="text" value="" style="width:10em" class="input-xlarge">';
    s += '</form>';
    s += '<form class="form-inline">';
    s += '  <label for="inputHostName">' + gettext("Host name") + '</label>';
    s += '  <input id="inputHostName" type="text" value="" style="width:10em" class="input-xlarge">';
    s += '  <label for="inputIpAddress">' + gettext("IP address") + '</label>';
    s += '  <input id="inputIpAddress" type="text" value="" style="width:10em" class="input-xlarge">';
    s += '  <label for="inputPort">' + gettext("Port") + '</label>';
    s += '  <input id="inputPort" type="text" value="80" style="width:4em" class="input-xlarge">';
    s += '</form>';
    s += '<form class="form-inline" style="display:none;" id="dbNameArea">';
    s += '  <label for="inputDbName">' + gettext("DB name") + '</label>';
    s += '  <input id="inputDbName" type="text" value="" style="width:10em" class="input-xlarge">';
    s += '</form>';
    s += '<form class="form-inline">';
    s += '  <label for="inputUserName">' + gettext("User name") + '</label>';
    s += '  <input id="inputUserName" type="text" value="" style="width:10em" class="input-xlarge">';
    s += '  <label for="inputPassword">' + gettext("Password") + '</label>';
    s += '  <input id="inputPassword" type="password" value="" style="width:10em" class="input-xlarge">';
    s += '</form>';
    s += '<form class="form-inline">';
    s += '  <label for="inputPollingInterval">' + gettext("Polling interval (sec)") + '</label>';
    s += '  <input id="inputPollingInterval" type="text" value="30" style="width:4em;" class="input-xlarge">';
    s += '  <label for="inputRetryInterval">' + gettext("Retry interval (sec)") + '</label>';
    s += '  <input id="inputRetryInterval" type="text" value="10" style="width:4em;" class="input-xlarge">';
    s += '</form>';
    s += '</div>';
    return s;
  }
};

HatoholServerEditDialog.prototype.onAppendMainElement = function () {
  var self = this;

  self.fixupApplyButtonState();

  $("#inputHostName").keyup(function() {
    self.fixupApplyButtonState();
  });

  $("#selectServerType").change(function() {
    var type = $("#selectServerType").val();
    if (type == hatohol.MONITORING_SYSTEM_ZABBIX) {
      self.setDBNameTextState(false);
    } else if (type == hatohol.MONITORING_SYSTEM_NAGIOS) {
      self.setDBNameTextState(true);
    }
  });

  $("#inputIpAddress").keyup(function() {
    self.fixupApplyButtonState();
  });

  $("#inputNickName").keyup(function() {
    self.fixupApplyButtonState();
  });

  $("#inputPort").keyup(function() {
    self.fixupApplyButtonState();
  });

  $("#inputPollingInterval").keyup(function() {
    self.fixupApplyButtonState();
  });

  $("#inputRetryInterval").keyup(function() {
    self.fixupApplyButtonState();
  });

  $("#inputUserName").keyup(function() {
    self.fixupApplyButtonState();
  });

  $("#inputPassword").keyup(function() {
    self.fixupApplyButtonState();
  });
};

HatoholServerEditDialog.prototype.setApplyButtonState = function(state) {
  var btn = $(".ui-dialog-buttonpane").find("button:contains(" +
              this.applyButtonTitle + ")");
  if (state) {
     btn.removeAttr("disabled");
     btn.removeClass("ui-state-disabled");
  } else {
     btn.attr("disabled", "disable");
     btn.addClass("ui-state-disabled");
  }
};

HatoholServerEditDialog.prototype.fixupApplyButtonState = function(enable) {
  var self = this;
  var validHostName = !!$("#inputHostName").val();
  var validIpAddress = !!$("#inputIpAddress").val();
  var validNickName = !!$("#inputNickName").val();
  var validPort = !!$("#inputPort").val();
  var validPollingInterval = !!$("#inputPollingInterval").val();
  var validRetryInterval = !!$("#inputRetryInterval").val();
  var validUserName = !!$("#inputUserName").val();
  var validPassword = !!$("#inputPassword").val();
  var state =
    validHostName &&
    validIpAddress &&
    validNickName &&
    validPort &&
    validPollingInterval &&
    validRetryInterval &&
    validUserName &&
    validPassword;
  self.setApplyButtonState(state);
};

HatoholServerEditDialog.prototype.setDBNameTextState = function(state) {
  var dbNameArea = $("#dbNameArea");

  if (state) {
    dbNameArea.show();
  } else {
    dbNameArea.hide();
  }
};

HatoholServerEditDialog.prototype.setServer = function(server) {
  this.server = server;
  $("#selectServerType").val(server.type);
  $("#inputNickName").val(server.nickname);
  $("#inputHostName").val(server.hostName);
  $("#inputIpAddress").val(server.ipAddress);
  $("#inputPort").val(server.port);
  $("#inputUserName").val(server.userName);
  $("#inputPassword").val(server.password);
  $("#inputDbName").val(server.dbName);
  $("#inputPollingInterval").val(server.pollingInterval);
  $("#inputRetryInterval").val(server.retryInterval);

  this.setDBNameTextState(server.type == hatohol.MONITORING_SYSTEM_NAGIOS);
  this.fixupApplyButtonState();
};
