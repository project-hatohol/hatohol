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

var HatoholDialog = function(id, dialogTitle, elem, buttons) {

  var self = this;
  self.mainFrame = $("<div>");
  self.mainFrame.attr("id", id);
  self.mainFrame.append(elem);
  $("body").append(self.mainFrame);

  var dialogId = "#" + id
  $(dialogId).dialog({
    autoOpen: false,
    title: dialogTitle,
    closeOnEscape: false,
    width:  "95%",
    modal: true,
    buttons: buttons,
  });

  $(dialogId).dialog("open");
}
