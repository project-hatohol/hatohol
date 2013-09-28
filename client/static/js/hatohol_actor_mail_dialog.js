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

var HatoholActorMailDialog = function(applyCallback, currCommand) {
  var self = this;
  self.applyCallback = applyCallback;
  self.currCommand = currCommand;

  var dialogButtons = [{
    text: gettext("APPLY"),
    click: function() { self.okButtonClicked() },
  }, {
    text: gettext("CANCEL"),
    click: function() { self.closeDialog(); },
  }];

  // call the constructor of the super class
  var id = "hatohol_actor_mail_dialog"
  var title = "Execution parameter maker";
  HatoholDialog.apply(this, [id, title, dialogButtons]);
  self.setApplyButtonState(false);
}

HatoholActorMailDialog.prototype = Object.create(HatoholDialog.prototype);
HatoholActorMailDialog.prototype.constructor = HatoholActorMailDialog;

HatoholActorMailDialog.prototype.createMainElement = function() {
  var div = $(makeMainDivHTML());
  return div;

  function makeMainDivHTML() {
    var s = "";
    s += '<form class="form-inline">'
    s += '  <label for="inputTo">' + gettext("TO: ") + '</label>'
    s += '  <input id="inputTo" type="text" value="" style="height:1.8em;" class="input-xxlarge">'
    s += '</form>'
    s += '<form class="form-inline">'
    s += '  <label for="inputSmtpServer">' + gettext("SMTP server ") + '</label>'
    s += '  <label for="inputSmtpServer">' + gettext("(If empty, localhost is used)") + '</label>'
    s += '  <br>'
    s += '  <input id="inputSmtpServer" type="text" value="" style="height:1.8em;" class="input-xxlarge">'
    s += '</form>'
    return s;
  }
}

HatoholActorMailDialog.prototype.onAppendMainElement = function () {
  var self = this;

  $("#inputTo").keyup(function() {
    if ($("#inputTo").val())
      self.setApplyButtonState(true);
    else
      self.setApplyButtonState(false);
  });
}

HatoholActorMailDialog.prototype.okButtonClicked = function() {
  var toAddr = $("#inputTo").val();
  var commandDesc = "hatohol-actor-mail " + toAddr;
  var smtpServer = $("#inputSmtpServer").val();
  if (smtpServer)
    commandDesc += " --smtp-server " + smtpServer;

  this.applyCallback(ACTION_COMMAND, commandDesc);
  this.closeDialog();
}

HatoholActorMailDialog.prototype.setApplyButtonState = function(state) {
  var btn = $(".ui-dialog-buttonpane").find("button:contains(" +
              gettext("APPLY") + ")");
  if (state) {
     btn.removeAttr("disabled");
     btn.removeClass("ui-state-disabled");
  } else {
     btn.attr("disabled", "disable");
     btn.addClass("ui-state-disabled");
  }
}
