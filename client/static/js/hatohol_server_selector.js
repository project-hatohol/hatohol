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

var HatoholServerSelector = function(selectedCb) {

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
    this, ["server-selector", "Server selecion", dialogButtons]);
  setSelectButtonState(false);
  getServerList();

  function selectButtonClickedCb() {
    if (selectedCb) {
      if (!self.selectedRow)
        selectedCb(null);
      else
        selectedCb(self.serverArray[self.selectedRow.index()]);
    }
    $(this).dialog("close");
    $("#server-selector").remove();
  }

  function cancelButtonClickedCb() {
    if (selectedCb)
      selectedCb(null);
    $(this).dialog("close");
    $("#server-selector").remove();
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
    '<table class="table table-condensed table-striped table-hover" id="serverSelectTable">' +
    '  <thead>' +
    '    <tr>' +
    '      <th>ID</th>' +
    '      <th>' + gettext("Type") + '</th>' +
    '      <th>' + gettext("Hostname") + '</th>' +
    '      <th>' + gettext("IP Address") + '</th>' +
    '      <th>' + gettext("Nickname") + '</th>' +
    '    </tr>' +
    '  </thead>' +
    '  <tbody></tbody>' +
    '</table>'
    return html;
  }

  function makeTableBody(reply) {
    var s;
    self.serverArray = reply.servers;
    for (var i = 0; i < reply.servers.length; i++) {
      sv = reply.servers[i];
      s += '<tr>';
      s += '<td>' + sv.id + '</td>';
      s += '<td>' + sv.type + '</td>';
      s += '<td>' + sv.hostName + '</td>';
      s += '<td>' + sv.ipAddress + '</td>';
      s += '<td>' + sv.nickname  + '</td>';
      s += '</tr>';
    }
    return s;
  }

  function getServerList() {
    $.ajax({
      url: "/tunnel/server",
      type: "GET",
      success: function(reply) {
        var replyParser = new HatoholReplyParser(reply);
        if (!(replyParser.getStatus() === REPLY_STATUS.OK)) {
          $("#serverSelectMsgArea").text(replyParser.getStatusMessage());
          return;
        }

        // create a table
        var table = $(makeTable(reply));
        self.replaceMainElement(table);
        $("#serverSelectTable tbody").empty();
        $("#serverSelectTable tbody").append(makeTableBody(reply));

        // set events
        $("#serverSelectTable tr").click(function(){
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
        $("#serverSelectMsgArea").text(errorMsg);
      }
    })
  }
}

HatoholServerSelector.prototype = Object.create(HatoholDialog.prototype);
HatoholServerSelector.prototype.constructor = HatoholServerSelector;

HatoholServerSelector.prototype.createMainElement = function() {
  var ptag = $("<p/>");
  ptag.attr("id", "serverSelectMsgArea");
  ptag.text(gettext("Now getting server information..."));
  return ptag;
}
