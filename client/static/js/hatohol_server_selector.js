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

var HatoholServerSelector = function() {

    this.mainDiv = document.createElement("div");
    this.mainDiv.id = "serverSelectMainDiv";
    HatoholDialog("server-selector", "Server selecion", this.mainDiv);
    showInitialView();
    getServerList();

    function showInitialView() {
      $("#serverSelectMainDiv").html(
        '<p id="serverSelectMsgArea">' + gettext("Now getting servers...") +
        '</p>');
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

    function makeTableBody(data) {
      var s;
      for (var i = 0; i < data.servers.length; i++) {
        sv = data.servers[i];
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

    function parseReceivedServersJson(data) {
      // TODO: implement
      return true;
    }

    function getServerList() {
      $.ajax({
        url: "/tunnel/server",
        type: "GET",
        success: function(data) {
          if (!parseReceivedServersJson(data))
            return;
          $("#serverSelectMainDiv").html(makeTable(data));
          $("#serverSelectMainDiv tbody").empty();
          $("#serverSelectmainDiv tbody").append(makeTableBody(data));
        },
        error: function(XMLHttpRequest, textStatus, errorThrown) {
          console.log("ERROR: " + textStatus);
        }
      })
    }
}
