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
  self.queryData = {"serverId": serverId};

  // call the constructor of the super class
  HatoholSelectorDialog.apply(
    this, ["host-selector", gettext("Host selecion"), selectedCb]);
  self.start("/tunnel/host", "GET");
}

HatoholHostSelector.prototype =
  Object.create(HatoholSelectorDialog.prototype);
HatoholHostSelector.prototype.constructor = HatoholHostSelector;

HatoholHostSelector.prototype.makeQueryData = function() {
    return this.queryData;
}

HatoholHostSelector.prototype.getNumberOfObjects = function(reply) {
  return reply.numberOfHosts;
}

HatoholHostSelector.prototype.generateMainTable = function(tableId) {
  var html =
  '<table class="table table-condensed table-striped table-hover" id=' +
  tableId + '>' +
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

HatoholHostSelector.prototype.generateTableRows = function(reply) {
  var s = "";
  this.setObjectArray(reply.hosts);
  for (var i = 0; i < reply.hosts.length; i++) {
    host = reply.hosts[i];
    s += '<tr>';
    s += '<td>' + host.id + '</td>';
    s += '<td>' + host.serverId + '</td>';
    s += '<td>' + host.hostName + '</td>';
    s += '</tr>';
  }
  return s;
}
