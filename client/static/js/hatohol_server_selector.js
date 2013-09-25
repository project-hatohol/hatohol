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
  self.start("/tunnel/server", "GET");
}

HatoholServerSelector.prototype =
  Object.create(HatoholSelectorDialog.prototype);
HatoholServerSelector.prototype.constructor = HatoholServerSelector;

HatoholServerSelector.prototype.getNumberOfObjects = function(reply) {
  return reply.numberOfServers;
}

HatoholServerSelector.prototype.generateMainTable = function(tableId) {
  var html =
  '<table class="table table-condensed table-striped table-hover" id=' +
  tableId + '>' +
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

HatoholServerSelector.prototype.generateTableRows = function(reply) {
  var s = "";
  this.setObjectArray(reply.servers);
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
