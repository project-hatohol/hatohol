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

var HatoholHostgroupDialog = function(serverId, selectedCb) {
  var self = this;
  self.queryData = {"serverId": serverId};

  // call the constructor of the super class
  HatoholSelectorDialog.apply(
    this, ["host-selector", gettext("Host selecion"), selectedCb]);
  self.start("/hostgroup", "GET");
}

HatoholHostgroupDialog.prototype =
  Object.create(HatoholSelectorDialog.prototype);
HatoholHostgroupDialog.prototype.constructor = HatoholHostgroupDialog;

HatoholHostgroupDialog.prototype.makeQueryData = function() {
    return this.queryData;
}

HatoholHostgroupDialog.prototype.getNumberOfObjects = function(reply) {
  return reply.numberOfHosts;
}

HatoholHostgroupDialog.prototype.generateMainTable = function(tableId) {
  var html =
  '<table class="table table-condensed table-striped table-hover" id=' +
  tableId + '>' +
  '  <thead>' +
  '    <tr>' +
  '      <th>' + gettext("Server ID") + '</th>' +
  '      <th>GroupID</th>' +
  '      <th>HostgroupName</th>' +
  '    </tr>' +
  '  </thead>' +
  '  <tbody></tbody>' +
  '</table>'
  return html;
}

HatoholHostgroupDialog.prototype.generateTableRows = function(reply) {
  var s = "";
  this.setObjectArray(reply.hostgroups);
  for (var i = 0; i < reply.hostgroups.length; i++) {
    hostgroup = reply.hostgroups[i];
    s += '<tr>';
    s += '<td>' + hostgroup.serverId + '</td>';
    s += '<td>' + hostgroup.id + '</td>';
    s += '<td>' + hostgroup.hostName + '</td>';
    s += '</tr>';
  }
  return s;
}
