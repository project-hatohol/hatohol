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

var HatoholMessageBox = function(msg, title, buttonLabel) {

  var self = this;
  var label;
  if (buttonLabel)
    label = buttonLabel;
  else
    label = gettext("CLOSE");

  var button = [{
    text: label,
    click: function() {
      $(this).dialog("destroy");
      $(self.dialogId).remove();
    }
  }];
  var id = "hatohol-message-box";
  var div = "<div id='" + id + "'>" + msg + "</div>";
  $("body").append(div);

  self.dialogId = "#" + id
  $(self.dialogId).dialog({
    autoOpen: false,
    title: title,
    closeOnEscape: false,
    modal: true,
    buttons: button,
    open: function(event, ui){
      $(".ui-dialog-titlebar-close").hide();
    }
  });

  $(self.dialogId).dialog("open");
};

function hatoholInfoMsgBox(msg) {
  new HatoholMessageBox(msg, gettext("Information"));
};

function hatoholWarnMsgBox(msg) {
  new HatoholMessageBox(msg, gettext("Warning"));
};

function hatoholErrorMsgBox(msg) {
  new HatoholMessageBox(msg, gettext("Error"));
};

function hatoholMsgBoxForParser(reply, parser, title) {
  var msg = gettext("Failed to parse the result. status code: ");
  msg += parser.getStatus();
  new HatoholMessageBox(msg, gettext("Error"));
};

function hatoholErrorMsgBoxForParser(reply, parser) {
  hatoholMessageBoxForParser(reply, parser, gettext("Error"));
};
