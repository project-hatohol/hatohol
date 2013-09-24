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

  // call the constructor of the super class
  HatoholSelectorDialog.apply(
    this, ["server-selector", gettext("Server selecion"), selectedCb]);
  getServerList();

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
    self.setObjectArray(reply.servers);
    for (var i = 0; i < reply.servers.length; i++) {
      sv = reply.servers[i];
      s += '<tr>';
      s += '<td>' + sv.id + '</td>';
      s += '<td>' + makeMonitoringSystemTypeLabel(sv.type) + '</td>';
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
          self.setMessage(replyParser.getStatusMessage());
          return;
        }
        if (reply.numberOfServers == 0) {
          self.setMessage(gettext("No data."));
          return;
        }

        // create a table
        var table = $(makeTable(reply));
        self.replaceMainElement(table);
        $("#serverSelectTable tbody").empty();
        $("#serverSelectTable tbody").append(makeTableBody(reply));

        // set events
        $("#serverSelectTable tr").click(function(){
          var selectedRow = self.getSelectedRow();
          if (selectedRow)
            selectedRow.removeClass("info");
          else
            self.setSelectButtonState(true);
          $(this).attr("class", "info");
          self.setSelectedRow($(this));
        });
      },
      error: function(XMLHttpRequest, textStatus, errorThrown) {
        var errorMsg = "Error: " + XMLHttpRequest.status + ": " +
                       XMLHttpRequest.statusText;
        self.setMessage(errorMsg);
      }
    })
  }
}

HatoholServerSelector.prototype =
  Object.create(HatoholSelectorDialog.prototype);
HatoholServerSelector.prototype.constructor = HatoholServerSelector;
