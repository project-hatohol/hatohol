/*
 * Copyright (C) 2013-2014 Project Hatohol
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

var ServersView = function(userProfile) {
  var self = this;
  var serverIds = [];
  var paramsArrayMap = {};
  var currServersMap = {};
 
  // call the constructor of the super class
  HatoholMonitoringView.apply(this, [userProfile]);

  if (userProfile.hasFlag(hatohol.OPPRVLG_CREATE_SERVER)) {
    $("#add-server-button").show();
//    TODO: We should fix this in 16.12 and later release.
//    $("#bulkupload-server-button").show();
  }
  if (userProfile.hasFlag(hatohol.OPPRVLG_DELETE_SERVER) ||
      userProfile.hasFlag(hatohol.OPPRVLG_DELETE_ALL_SERVER)) {
    $("#delete-server-button").show();
  }
  self.startConnection('server', updateCore);
  $("#update-tirgger-server-button").show();

  $("#add-server-button").click(function() {
    new HatoholServerEditDialogParameterized({
      operator: userProfile.user,
      succeededCallback: addOrUpdateSucceededCb
    });
  });

  $("#bulkupload-server-button").click(function() {
    new HatoholServerBulkUploadDialog({
      operator: userProfile.user,
      succeededCallback: addOrUpdateSucceededCb
    });
  });


  $("#delete-server-button").click(function() {
    var msg = gettext("Delete the selected items ?");
    hatoholNoYesMsgBox(msg, deleteServers);
  });

  $("#update-tirgger-server-button").click(function() {
    var msg = gettext("Reload the triggers from the selected itmes ?");
    hatoholNoYesMsgBox(msg, updateTriggerServers);
  });

  function addOrUpdateSucceededCb() {
    self.startConnection('server', updateCore);
  }

  function setupCheckbox() {
  }

  function setupEditButtons(reply)
  {
    var i, id, servers = reply["servers"], serversMap = {};

    for (i = 0; i < servers.length; ++i)
      serversMap[servers[i]["id"]] = servers[i];

    for (i = 0; i < servers.length; ++i) {
      id = "#edit-server" + servers[i]["id"];
      $(id).click(function() {
        var serverId = this.getAttribute("serverId");
        new HatoholServerEditDialogParameterized({
          operator: userProfile.user,
          succeededCallback: addOrUpdateSucceededCb,
          targetServer: serversMap[serverId]
        });
      });
    }

    if (userProfile.hasFlag(hatohol.OPPRVLG_UPDATE_SERVER) ||
        userProfile.hasFlag(hatohol.OPPRVLG_UPDATE_ALL_SERVER))
    {
      $(".edit-server-column").show();
    }
  }

  function parseResult(data) {
    var msg;
    var malformed =false;
    if (data.result === undefined)
      malformed = true;
    if (!malformed && !data.result && data.message === undefined)
      malformed = true;
    if (malformed) {
      msg = "The returned content is malformed: " +
        "Not found 'result' or 'message'.\n" +
        JSON.stringify(data);
      hatoholErrorMsgBox(msg);
      return false;
    }
    if (!data.result) {
      msg = "Failed:\n" + data.message;
      hatoholErrorMsgBox(msg);
      return false;
    }
    if (data.id === undefined) {
      msg = "The returned content is malformed: " +
        "'result' is true, however, 'id' missing.\n" +
        JSON.stringfy(data);
      hatoholErrorMsgBox(msg);
      return false;
    }
    return true;
  }

  function deleteServers() {
    var delId = [], serverId;
    var checkbox = $(".selectcheckbox");
    $(this).dialog("close");
    for (var i = 0; i < checkbox.length; i++) {
      if (!checkbox[i].checked)
        continue;
      serverId = checkbox[i].getAttribute("serverId");
      delId.push(serverId);
    }
    new HatoholItemRemover({
      id: delId,
      type: "server",
      completionCallback: function() {
        self.startConnection("server", updateCore);
      },
    });
    hatoholInfoMsgBox("Deleting...", {buttons: []});
  }

  function updateTriggerServers() {
    var updateIds = [], serverId;
    var checkbox = $(".selectcheckbox");
    $(this).dialog("close");
    for (var i = 0; i < checkbox.length; i++) {
      if (!checkbox[i].checked)
        continue;
      serverId = checkbox[i].getAttribute("serverId");
      updateIds.push(serverId);
    }
    new HatoholItemUpdate({
      id: updateIds,
      type: "server-trigger",
      completionCallback: function() {
        self.startConnection("server", updateCore);
      },
    });
    hatoholInfoMsgBox("Reloading...", {buttons: []});
  }

  function drawTableBody(rd) {
    var x;
    var s;
    var o;
    var ip, serverURL;

    s = "";
    for (x = 0; x < rd["servers"].length; ++x) {
      o = rd["servers"][x];
      ip = o["ipAddress"];

      var serverId = o["id"];
      serverIds.push(serverId);
      var idConnStat = getIdConnStat(serverId);
      serverURL = getServerLocation(o);
      s += "<tr>";
      s += "<td class='delete-selector' style='display:none;'>";
      s += "<input type='checkbox' class='selectcheckbox' serverId='" + escapeHTML(o["id"]) + "'>";
      s += "</td>";
      s += "<td id='"+ idConnStat + "' " +
             "data-title='" + gettext("Server ID") + ": " + serverId + "'" +
             "data-html=true " +
             "data-trigger='hover' " +
             "data-container='body' " +
           ">" + escapeHTML(gettext("Checking")) + "</td>";
      s += "<td>" + makeMonitoringSystemTypeLabel(o) + "</td>";

      if (serverURL) {
        s += "<td class='server-url-link'><a href='" + serverURL +
          "' target='_blank'>" + escapeHTML(o["nickname"])  + "</a></td>";
      } else {
        s += "<td>" + escapeHTML(o["nickname"])  + "</td>";
      }
      s += "<td>" + escapeHTML(o["baseURL"])  + "</td>";


      s += "<td id='server-info-" + escapeHTML(serverId) + "'" +
           "  data-title='" + gettext("Server ID") + ": " + escapeHTML(serverId) + "'" +
           "  data-html=true" +
           "  data-trigger='hover'" +
           "  data-container='body'" +
           "  data-placement='left'" +
           "  serverType='" + getServerTypeId(o) + "'" +
           "  serverId='" + escapeHTML(serverId) + "'" +
           ">";
      s += "<i class='glyphicon glyphicon-info-sign'></i>";
      s += "</td>";
      s += "<td class='edit-server-column' style='display:none;'>";
      s += "<input id='edit-server" + escapeHTML(o["id"]) + "'";
      s += "  type='button' class='btn btn-default'";
      s += "  serverId='" + escapeHTML(o["id"]) + "'";
      s += "  value='" + gettext("EDIT") + "' />";
      s += "</td>";
      s += "</tr>";
    }

    return s;
  }

  function updateCore(reply) {
    $("#table tbody").empty();
    $("#table tbody").append(drawTableBody(reply));
    self.setupCheckboxForDelete($("#delete-server-button"));
    self.setupCheckboxForDelete($("#update-tirgger-server-button"));
    $(".delete-selector").show();
    setupEditButtons(reply);
    setupServerInfo(reply);
    self.displayUpdateTime();
    self.startConnection("server-conn-stat", updateServerConnStat);
  }

  function getIdConnStat(serverId) {
    var idConnStat = "connStat-" + serverId;
    return idConnStat;
  }

  function setStatusFailure(serverId) {
    var idConnStat = getIdConnStat(serverId);
    var label = gettext("Failed to get");
    $("#" + idConnStat).html(label);
  }

  function setStatusFailureAll() {
    for (var i = 0; i < serverIds.length; i++)
      setStatusFailure(serverIds[i]);
  }

  function updateServerConnStat(reply) {
    var connStatParser = new ServerConnStatParser(reply);
    if (connStatParser.isBadPacket()) {
        setStatusFailureAll();
        return;
    }
    for (var i = 0; i < serverIds.length; i++) {
      var serverId = serverIds[i];
      if (!connStatParser.setServerId(serverId)) {
        setStatusFailure(serverId);
        continue;
      }
      var label = connStatParser.getStatusLabel();
      var msgClass = connStatParser.getStatusLabel();
      var html = "<span class='" + label.msgClass + "'>" + label.msg + "</span>";

      var idConnStat = getIdConnStat(serverId);
      $("#" + idConnStat).html(html);
      options = {content: connStatParser.getInfoHTML()};
      $("#" + idConnStat).popover(options);
    }
  }

  function setupServerInfo(reply) {
    var servers = reply["servers"];
    for (var i = 0; i < servers.length; ++i) {
      currServersMap[servers[i]["id"]] = servers[i];
    }
    getServerTypesAsync();
  }

  function getServerTypesAsync() {
    new HatoholConnector({
      url: "/server-type",
      replyCallback: replyCallback,
    });
  }

  function replyCallback(reply, parser) {
    if (!(reply.serverType instanceof Array)) {
      return;
    }
    paramsArrayMap = {};
    for (var i = 0; i < reply.serverType.length; i++) {
      var serverTypeInfo = reply.serverType[i];
      var name = serverTypeInfo.name;
      if (!name) {
        continue;
      }
      var type = serverTypeInfo.type;
      if (type == hatohol.MONITORING_SYSTEM_HAPI2)
        type = serverTypeInfo.uuid;
      if (type === undefined) {
        continue;
      }
      var parameters = serverTypeInfo.parameters;
      if (parameters === undefined) {
        continue;
      }
      paramsArrayMap[type] = JSON.parse(parameters);
    }
    makeServerInfo();
  }

  function makeServerInfo() {
    for (var i = 0; i < serverIds.length; i++) {
      var id = "#server-info-" + serverIds[i];
      $(id).popover({
        content: function() {
          var serverId = this.getAttribute("serverId");
          var server = currServersMap[serverId];
          var serverType = this.getAttribute("serverType");
          var paramsArray = paramsArrayMap[serverType];
          var s = "";
          s += gettext('Monitoring server type') + ": " +
            makeMonitoringSystemTypeLabel(server) + "</br>";
          for (var j = 0; j < paramsArray.length; j++) {
            var param = paramsArray[j];
            var value;
            if (!param.label || !param.id || param.hidden || param.inputStyle == "password")
              continue;
            s += gettext(param.label) + ": ";
            if (param.inputStyle === "checkBox") {
              value = server[param.id];
              if (value === true) {
                s += gettext("True");
              } else {
                s += gettext("False");
              }
            } else {
              value = server[param.id];
              if (value) {
                s += value;
              } else {
                if (param.hint) {
                  s += param.hint;
                }
              }
            }
            s += "<br>";
          }
          return s;
        }
      });
    }
  }
};

ServersView.prototype = Object.create(HatoholMonitoringView.prototype);
ServersView.prototype.constructor = ServersView;

var ServerConnStatParser = function(reply) {
  var self = this;
  self.badPacket = false;
  if (!reply) {
    self.badPacket = true;
    return;
  }
  self.connStat = reply.serverConnStat;
  self.currServerId = null;
  self.currConnStat = null;

  if (!self.connStat) {
    self.badPacket = true;
    return;
  }
};

ServerConnStatParser.prototype.isBadPacket = function() {
    return this.badPacket;
};

ServerConnStatParser.prototype.setServerId = function(serverId) {
  var self = this;
  var key = serverId.toString();
  var connStat = self.connStat[key];
  if (!connStat)
    return false;
  self.currServerId = serverId;
  self.currConnStat = connStat;
  return true;
};

ServerConnStatParser.prototype.getStatusLabel = function() {
  var self = this;
  if (self.currConnStat === undefined)
    throw new Error("Called before a valid server ID is set.");
  var currStatNum = self.currConnStat.status;
  if (currStatNum === undefined)
    return gettext("N/A");

  switch(currStatNum) {
  case hatohol.ARM_WORK_STAT_INIT:
    return {msg:gettext("Initial State"), msgClass:"text-warning"};
  case hatohol.ARM_WORK_STAT_OK:
    return {msg:gettext("OK"), msgClass:"text-success"};
  case hatohol.ARM_WORK_STAT_FAILURE:
    return {msg:gettext("Error"), msgClass:"text-error"};
  default:
    return {msg:gettext("Unknown:") + currStatNum, msgClass:"text-error"};
  }
  throw new Error("This line must no be executed.");
};

ServerConnStatParser.prototype.getInfoHTML = function() {
  var self = this;
  if (self.currConnStat === undefined)
    throw new Error("Called before a valid server ID is set.");
  var s = "";

  // running
  var running = self.currConnStat.running;
  s += gettext("Running") + ": ";
  switch (running) {
  case 0:
    s += gettext("No");
    break;
  case 1:
    s += gettext("Yes");
    break;
  default:
    s += "Unknown: " + running;
    break;
  }

  // status update time
  s += "<br>";
  var statUpdateTime = self.unixTimeToVisible(self.currConnStat.statUpdateTime);
  s += gettext("Status update time") + ": " + statUpdateTime;

  // last success time
  s += "<br>";
  var lastSuccessTime = self.unixTimeToVisible(self.currConnStat.lastSuccessTime);
  s += gettext("Last success time") + ": " + lastSuccessTime;

  // last failure time
  s += "<br>";
  var lastFailureTime = self.unixTimeToVisible(self.currConnStat.lastFailureTime);
  s += gettext("Last failure time") + ": " + lastFailureTime;

  // number of updates
  s += "<br>";
  var numUpdate = self.currConnStat.numUpdate;
  if (numUpdate === undefined)
    s += "N/A";
  else
    s += gettext("Number of communication") + ": " + numUpdate;

  // number of failures
  s += "<br>";
  var numFailure = self.currConnStat.numFailure;
  if (numFailure === undefined)
    s += "N/A";
  else
    s += gettext("Number of failure") + ": " + numFailure;

  // last failure comment
  s += "<br>";
  s += gettext("Comment for the failure") + ": ";
  var failureComment = self.currConnStat.failureComment;
  if (failureComment === undefined)
    s += "N/A";
  else if (failureComment === "")
    s += "-";
  else
    s += failureComment;

  return s;
};

ServerConnStatParser.prototype.unixTimeToVisible = function(unixTimeString) {
  if (unixTimeString === undefined)
    return "N/A";
  var unixTime = Number(unixTimeString);
  if (unixTime === 0)
    return "-";
  var date = new Date(unixTime * 1000);
  var year  = date.getYear();
  var month = date.getMonth() + 1;
  var day   = date.getDate();
  if (year < 2000)
    year += 1900;
  if (month < 10)
    month = "0" + month;
  if (day < 10)
    day = "0" + day;
  var hour = date.getHours();
  var min  = date.getMinutes();
  var sec  = date.getSeconds();
  if (hour < 10)
    hour = "0" + hour;
  if (min < 10)
    min = "0" + min;
  if (sec < 10)
    sec = "0" + sec;

  var showString = year + "-" + month + "-" + day + " " +
                   hour + ":" + min + ":" + sec;
  return showString;
};
