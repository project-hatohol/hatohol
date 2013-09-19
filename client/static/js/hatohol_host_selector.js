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

var HatoholHostSelector = function(serverId, selectedCb) {
  var self = this;

  var dialogButtons = [{
    text: gettext("SELECT"),
    click: selectButtonClickedCb,
  }, {
    text: gettext("CANCEL"),
    click: cancelButtonClickedCb,
  }];

  // call the constructor of the super class
  HatoholDialog.apply(
    this, ["host-selector", gettext("Host selecion"), dialogButtons]);
  setSelectButtonState(false);
  getHostList();

  function selectButtonClickedCb() {
    if (selectedCb) {
      if (!self.selectedRow)
        selectedCb(null);
      else
        selectedCb(self.hostArray[self.selectedRow.index()]);
    }
    self.closeDialog();
  }

  function cancelButtonClickedCb() {
    if (selectedCb)
      selectedCb(null);
    self.closeDialog();
  }

  function setSelectButtonState(state) {
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

  function makeTable(data) {
    var html =
    '<table class="table table-condensed table-striped table-hover" id="hostSelectTable">' +
    '  <thead>' +
    '    <tr>' +
    '      <th>ID</th>' +
    '      <th>' + gettext("Server ID") + '</th>' +
    '      <th>' + gettext("Hostname") + '</th>' +
    '    </tr>' +
    '  </thead>' +
    '  <tbody></tbody>' +
    '</table>'
    return html;
  }

  function makeTableBody(reply) {
    var s;
    self.hostArray = reply.hosts;
    for (var i = 0; i < reply.hosts.length; i++) {
      host = reply.hosts[i];
      s += '<tr>';
      s += '<td>' + host.id + '</td>';
      s += '<td>' + host.serverId + '</td>';
      s += '<td>' + sv.hostName + '</td>';
      s += '</tr>';
    }
    return s;
  }

  function getHostList() {
    $.ajax({
      url: "/tunnel/host",
      type: "GET",
      data: {"serverId": serverId},
      success: function(reply) {
        var replyParser = new HatoholReplyParser(reply);
        if (!(replyParser.getStatus() === REPLY_STATUS.OK)) {
          $("#hostSelectMsgArea").text(replyParser.getStatusMessage());
          return;
        }
        if (reply.numberOfHosts == 0) {
          $("#hostSelectMsgArea").text(gettext("No data."));
          return;
        }

        // create a table
        var table = $(makeTable(reply));
        self.replaceMainElement(table);
        $("#hostSelectTable tbody").empty();
        $("#hostSelectTable tbody").append(makeTableBody(reply));

        // set events
        $("#hostSelectTable tr").click(function(){
          if (self.selectedRow)
            self.selectedRow.removeClass("info");
          else
            setSelectButtonState(true);
          $(this).attr("class", "info");
          self.selectedRow = $(this);
        });
      },
      error: function(XMLHttpRequest, textStatus, errorThrown) {
        var errorMsg = "Error: " + XMLHttpRequest.status + ": " +
                       XMLHttpRequest.statusText;
        $("#hostSelectMsgArea").text(errorMsg);
      }
    })
  }
}

HatoholHostSelector.prototype = Object.create(HatoholDialog.prototype);
HatoholHostSelector.prototype.constructor = HatoholHostSelector;

HatoholHostSelector.prototype.createMainElement = function() {
  var ptag = $("<p/>");
  ptag.attr("id", "hostSelectMsgArea");
  ptag.text(gettext("Now getting host information..."));
  return ptag;
}

