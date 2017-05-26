/*
 * Copyright (C) 2014 Project Hatohol
 *
 * This file is part of Hatohol.
 *
 * Hatohol is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License, version 3
 * as published by the Free Software Foundation.
 *
 * Hatohol is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with Hatohol. If not, see
 * <http://www.gnu.org/licenses/>.
 */

/*
 * HatoholLogSearchSystemEditor
 */
var HatoholLogSearchSystemEditor = function(params) {
  var self = this;

  self.logSearchSystem = params.logSearchSystem;
  self.succeededCallback = params.succeededCallback;
  self.windowTitle = self.logSearchSystem ?
    gettext("EDIT LOG SEARCH SYSTEM") :
    gettext("ADD LOG SEARCH SYSTEM");
  self.applyButtonTitle = self.logSearchSystem ?
    gettext("APPLY") : gettext("ADD");

  var dialogButtons = [{
    text: self.applyButtonTitle,
    click: applyButtonClickedCb
  }, {
    text: gettext("CANCEL"),
    click: cancelButtonClickedCb
  }];

  // call the constructor of the super class
  dialogAttrs = { width: "auto" };
  HatoholDialog.apply(
    this, ["log-search-system-editor", self.windowTitle,
           dialogButtons, dialogAttrs]);

  //
  // Dialog button handlers
  //
  function applyButtonClickedCb() {
    if (validateParameters()) {
      makeQueryData();
      if (self.logSearchSystem)
        hatoholInfoMsgBox(
	  gettext("Now updating the log search system ..."));
      else
        hatoholInfoMsgBox(
	  gettext("Now creating a log search system ..."));
      postLogSearchSystem();
    }
  }

  function makeQueryData() {
    var queryData = {};
    queryData.type = $("#selectLogSearchSystemType").val();
    queryData.base_url = $("#editLogSearchSystemBaseURL").val();
    return queryData;
  }

  function postLogSearchSystem() {
    var url = "/log-search-systems/";
    if (self.logSearchSystem)
      url += self.logSearchSystem.id;
    new HatoholConnector({
      pathPrefix: "",
      url: url,
      request: self.logSearchSystem ? "PUT" : "POST",
      data: makeQueryData(),
      replyCallback: replyCallback,
      parseErrorCallback: hatoholErrorMsgBoxForParser
    });
  }

  function replyCallback(reply, parser) {
    self.closeDialog();
    if (self.logSearchSystem)
      hatoholInfoMsgBox(gettext("Successfully updated."));
    else
      hatoholInfoMsgBox(gettext("Successfully created."));

    if (self.succeededCallback)
      self.succeededCallback();
  }

  function validateParameters() {
    if ($("#editLogSearchSystemType").val() === "") {
      hatoholErrorMsgBox(gettext("Type is empty!"));
      return false;
    }

    if ($("#editLogSearchSystemBaseURL").val() === "") {
      hatoholErrorMsgBox(gettext("Base URL is empty!"));
      return false;
    }

    return true;
  }

  function cancelButtonClickedCb() {
    self.closeDialog();
  }
};

HatoholLogSearchSystemEditor.prototype = Object.create(HatoholDialog.prototype);
HatoholLogSearchSystemEditor.prototype.constructor = HatoholLogSearchSystemEditor;

HatoholLogSearchSystemEditor.prototype.createMainElement = function() {
  return '' +
  '<div>' +
  '<div>' +
  '<label>' + gettext("Type") + '</label>' +
  '<select id="selectLogSearchSystemType" style="width:10em">' +
  '  <option value="' + "groonga" + '">' + gettext("Groonga") + '</option>' +
  '</select>' +
  '</div>' +
  '<div>' +
  '<label for="editLogSearchSystemBaseURL">' + gettext("Base URL") + '</label>' +
  '<input id="editLogSearchSystemBaseURL" type="text" ' +
  '       class="input-xlarge">' +
  '</div>' +
  '</div>';
};

HatoholLogSearchSystemEditor.prototype.onAppendMainElement = function() {
  this.setLogSearchSystem(this.logSearchSystem);
  this.resetWidgetState();
};

HatoholLogSearchSystemEditor.prototype.resetWidgetState = function() {
};

HatoholLogSearchSystemEditor.prototype.setLogSearchSystem = function(system) {
  this.logSearchSystem = system;
  $("#selectLogSearchSystemType").val(system ? system.type : "groonga");
  $("#editLogSearchSystemBaseURL").val(system ? system.base_url : "");
};
