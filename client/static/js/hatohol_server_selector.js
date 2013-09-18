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

    HatoholDialog("server-selector", "Server selecion", makeTable());

    function makeTable() {
      var div = document.createElement("div");
      div.innerHTML =
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
      '  <tbody>' +
      '  </tbody>' +
      '</table>'
      return div;
    }
}
