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

var HatoholServerEditDialog = function(succeededCb) {
	var self = this;

	var dialogButtons = [{
		text: gettext("ADD"),
		click: addButtonClickedCb
	}, {
		text: gettext("CANCEL"),
		click: cancelButtonClickedCb
	}];

	// call the constructor of the super class
	dialogAttrs = { width: "auto" };
	HatoholDialog.apply(
			this, ["server-edit-dialog", gettext("ADD SERVER"), dialogButtons, dialogAttrs]);
	self.setAddButtonState(false);

	//
	// Dialog button handlers
	//
	function addButtonClickedCb() {
		if (validateParameters()) {
			makeQueryData();
			hatoholInfoMsgBox(gettext("Now adding a server..."));
			postAddAction();
		}
	}

	function cancelButtonClickedCb() {
		self.closeDialog();
	}

	function makeQueryData() {
		var queryData = {};
		queryData.serverid = $("#inputServerId").val();
		queryData.type = $("#selectServerType").val();
		queryData.hostname = $("#inputHostName").val();
		queryData.ipaddress = $("#inputIpAddress").val();
		queryData.nickname = $("#inputNickName").val();
		queryData.port = $("#inputPort").val();
		queryData.polling = $("#inputPollingInterval").val();
		queryData.retry = $("#inputRetryInterval").val();
		queryData.user = $("#inputUserName").val();
		queryData.password = $("#inputPassword").val();
		queryData.dbname = $("#inputDbName").val();
		return queryData;
	}

	function postAddAction() {
		new HatoholConnector({
			url: "/server",
			request: "POST",
			data: makeQueryData(),
			replyCallback: replyCallback,
			parseErrorCallback: hatoholErrorMsgBoxForParser
		});
	}
	
	function replyCallback(reply, parser) {
		self.closeDialog();
		hatoholInfoMsgBox(gettext("Successfully created."));

		if (succeededCb)
			succeededCb();
	}

	function validateParameters() {
		var type = $("#selectServerTyepe").val();

		if ($("#inputServerId").val() == "") {
			hatoholErrorMsgBox(gettext("Server id is empty!"));
			return false;
		}
		if (type == "Zabbix" && type == "Nagios") {
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
			hatoholErrorMsgBox(gettext("Port is empty!")); return false;
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
		if (type == "nagios" && $("#inputDbName").val() == "") {
			hatoholErrorMsgBox(gettext("DB name is empty!"));
			return false;
		} else if (type == "zabbix" && $("#inputDbName").val() != "") {
			hatoholErrorMsgBox(gettext(
						"When select Zabbix from server type, you needn't input a DB name."));
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
		s += '<label for="inputServerId">' + gettext("Server ID") + '</label>';
		s += '<input id="inputServerId" type="text" value="" style="height:1.8em;" class="input-xlarge">';
		s += '<label>' + gettext("Server type") + '</label>';
		s += '<select id="selectServetType">';
		s += '	<option value="zabbix">' + gettext("Zabbix") + '</option>';
		s += '	<option value="nagios">' + gettext("Nagios") + '</option>';
		s += '</select>';
		s += '<label for="inputHostName">' + gettext("Host name") + '</label>';
		s += '<input id="inputHostName" type="text" value="" style="height:1.8em;" class="input-xlarge">';
		s += '<label for="inputIpAddress">' + gettext("IP address") + '</label>';
		s += '<input id="inputIpAddress" type="text" value="" style="height:1.8em;" class="input-xlarge">';
		s += '<label for="inputNickName">' + gettext("Nickname") + '</label>';
		s += '<input id="inputNickName" type="text" value="" style="height:1.8em;" class="input-xlarge">';
		s += '<label for="inputPort">' + gettext("Port") + '</label>';
		s += '<input id="inputPort" type="text" value="" style="height:1.8em;" class="input-xlarge">';
		s += '<label for="inputPollingInterval">' + gettext("Polling interval") + '</label>';
		s += '<input id="inputPollingInterval" type="text" value="" style="height:1.8em;" class="input-xlarge">';
		s += '<label for="inputRetryInterval">' + gettext("Retry interval") + '</label>';
		s += '<input id="inputRetryInterval" type="text" value="" style="height:1.8em;" class="input-xlarge">';
		s += '<label for="inputUserName">' + gettext("User name") + '</label>';
		s += '<input id="inputUserName" type="text" value="" style="height:1.8em;" class="input-xlarge">';
		s += '<label for="inputPassword">' + gettext("Password") + '</label>';
		s += '<input id="inputPassword" type="password" value="" style="height:1.8em;" class="input-xlarge">';
		s += '<label for="inputDbName">' + gettext("DB name") + '</label>';
		s += '<input id="inputDbName" type="text" value="" style="height:1.8em;" class="input-xlarge">';
		s += '</div>';
		return s;
	}
};

HatoholServerEditDialog.prototype.onAppendMainElement = function () {
	var self = this;
	validServerId = false;
	validHostName = false;
	validIpAddress = false;
	validNickName = false;
	validPort = false;
	validPollingInterval = false;
	validRetryInterval = false;
	validUserName = false;
	validPassword = false;

  $("#inputServerId").keyup(function() {
    validServerId = !!$("#inputServerId").val();
    fixupAddButtonState();
  });

  $("#inputHostName").keyup(function() {
    validHostName = !!$("#inputHostName").val();
    fixupAddButtonState(); });

  $("#inputIpAddress").keyup(function() {
    validIpAddress = !!$("#inputIpAddress").val();
    fixupAddButtonState();
  });

  $("#inputNickName").keyup(function() {
    validNickName = !!$("#inputNickName").val();
    fixupAddButtonState();
  });

  $("#inputPort").keyup(function() {
    validPort = !!$("#inputPort").val();
    fixupAddButtonState();
  });

  $("#inputPollingInterval").keyup(function() {
    validPollingInterval = !!$("#inputPollingInterval").val();
    fixupAddButtonState();
  });

  $("#inputRetryInterval").keyup(function() {
    validRetryInterval = !!$("#inputRetryInterval").val();
    fixupAddButtonState();
  });

  $("#inputUserName").keyup(function() {
    validUserName = !!$("#inputUserName").val();
    fixupAddButtonState();
  });

  $("#inputPassword").keyup(function() {
    validPassword = !!$("#inputPassword").val();
    fixupAddButtonState();
  });

	function fixupAddButtonState() {
		var state = (
				validServerId &&
				validHostName &&
				validIpAddress &&
				validNickName &&
				validPort &&
				validPollingInterval &&
				validRetryInterval &&
				validUserName &&
				validPassword);
		self.setAddButtonState(state);
	}
};

HatoholServerEditDialog.prototype.setAddButtonState = function(state) {
  var btn = $(".ui-dialog-buttonpane").find("button:contains(" +
              gettext("ADD") + ")");
  if (state) {
     btn.removeAttr("disabled");
     btn.removeClass("ui-state-disabled");
  } else {
     btn.attr("disabled", "disable");
     btn.addClass("ui-state-disabled");
  }
};
