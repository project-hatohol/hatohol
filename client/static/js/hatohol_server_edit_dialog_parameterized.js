/*
 * Copyright (C) 2013-2014 Project Hatohol
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

var HatoholServerEditDialogParameterized = function(params) {
  var self = this;

  self.operator = params.operator;
  self.server = params.targetServer;
  self.succeededCallback = params.succeededCallback;
  self.windowTitle =
    self.server ? gettext("EDIT SERVER") : gettext("ADD SERVER");
  self.applyButtonTitle = self.server ? gettext("APPLY") : gettext("ADD");

  var dialogButtons = [{
    text: self.applyButtonTitle,
    click: addButtonClickedCb
  }, {
    text: gettext("CANCEL"),
    click: cancelButtonClickedCb
  }];

  // call the constructor of the super class
  var dialogAttrs = {width: 768};
  HatoholDialog.apply(
      this, ["server-edit-dialog", self.windowTitle,
             dialogButtons, dialogAttrs]);

  // set initial state
  if (self.server)
    self.setServer(self.server);
  self.fixupApplyButtonState();

  //
  // Dialog button handlers
  //
  function addButtonClickedCb() {
    if (self.server)
      hatoholInfoMsgBox(gettext("Now updating the server ..."));
    else
      hatoholInfoMsgBox(gettext("Now adding a server..."));
    postAddServer();
  }

  function cancelButtonClickedCb() {
    self.closeDialog();
  }

  function makeQueryData() {
    var paramObj = self.currParamObj;
    var type = $("#selectServerType").val();
    var queryData = {'type':type, '_extra':{}};
    for (var i = 0; i < paramObj.length; i++) {
      var param = paramObj[i];

      var val;
      if (param.inputStyle == 'checkBox')
        val = $('#' + param.id).prop('checked');
      else
        val = $('#' + param.id).val();

      if (param.class)
        queryData[param.class] = val;
      else
        queryData['_extra'][param.name] = val;
    }
    return queryData;
  }

  function postAddServer() {
    var url = "/server";
    if (self.server)
      url += "/" + self.server.id;
    new HatoholConnector({
      url: url,
      request: self.server ? "PUT" : "POST",
      data: makeQueryData(),
      replyCallback: replyCallback,
      parseErrorCallback: hatoholErrorMsgBoxForParser
    });
  }

  function replyCallback(reply, parser) {
    self.closeDialog();
    if (self.server)
      hatoholInfoMsgBox(gettext("Successfully updated."));
    else
      hatoholInfoMsgBox(gettext("Successfully created."));

    if (self.succeededCallback)
      self.succeededCallback();
  }
};

HatoholServerEditDialogParameterized.prototype = Object.create(HatoholDialog.prototype);
HatoholServerEditDialogParameterized.prototype.constructor = HatoholServerEditDialogParameterized;

HatoholServerEditDialogParameterized.prototype.createMainElement = function() {
  var self = this;
  getServerTypesAsync();
  return makeMainDiv();

  function getServerTypesAsync() {
    new HatoholConnector({
      url: "/server-type",
      replyCallback: replyCallback,
      parseErrorCallback: hatoholErrorMsgBoxForParser
    });
  }

  function replyCallback(reply, parser) {
    if (!(reply.serverType instanceof Array)) {
      hatoholErrorMsgBox("[Malformed reply] Not found array: serverType");
      return;
    }
    self.paramArray = []
    for (var i = 0; i < reply.serverType.length; i ++) {
      var serverTypeInfo = reply.serverType[i];
      var name = serverTypeInfo.name;
      if (!name) {
        hatoholErrorMsgBox("[Malformed reply] Not found element: name");
        return;
      }
      var type = serverTypeInfo.type;
      if (type == undefined) {
        hatoholErrorMsgBox("[Malformed reply] Not found element: type");
        return;
      }

      var parameters = serverTypeInfo.parameters;
      if (parameters == undefined) {
        hatoholErrorMsgBox("[Malformed reply] Not found element: parameters");
        return;
      }

      $('#selectServerType').append($('<option>').html(name).val(type));
      self.paramArray[type] = parameters;
    }
  }

  function makeMainDiv() {
    var mainDiv = $('<div id="add-server-div">');
    var form = $('<form class="form-inline">').appendTo(mainDiv);
    form.append($('<label>').text(gettext('Server type')));
    var select = $('<select id="selectServerType">').appendTo(form);
    select.append($('<option>').html('Please select').val('_header'));
    mainDiv.append('<form id="add-server-param-form" class="form-horizontal" role="form">');
    return mainDiv;
  }
};

HatoholServerEditDialogParameterized.prototype.onAppendMainElement = function () {
  var self = this;

  self.fixupApplyButtonState();

  $('#selectServerType').change(function() {
    self.currParamObj = undefined;
    $('#add-server-param-form').empty();
    var type = $("#selectServerType").val();
    if (type != "_header") {
      // We assume that type is NULL when 'Please select' is selected.
      var paramObj = setupParametersForms(self.paramArray[type]);
      self.currParamObj = paramObj;
    }
    self.fixupApplyButtonState();
  });

  function setupParametersForms(parameters) {
    paramObj = JSON.parse(parameters);
    if (!(paramObj instanceof Array)) {
        hatoholErrorMsgBox("[Malformed reply] parameters is not array");
        return;
    }

    var s = '';
    for (var i = 0; i < paramObj.length; i++)
      s += makeFormHTMLOfOneParameter(paramObj[i], i);
    $('#add-server-param-form').append(s);

    // set events to fix up state of 'apply' button
    for (var i = 0; i < paramObj.length; i++) {
      $('#' + paramObj[i].id).keyup(function() {
        self.fixupApplyButtonState();
      });
    }
    return paramObj;
  }

  function makeFormHTMLOfOneParameter(param, index) {
    var s = '';
    var label = param.name;

    var defaultValue = '';
    if (param.default != undefined)
      defaultValue = param.default

    var inputStyle = param.inputStyle;
    if (inputStyle == undefined)
      inputStyle = 'text';

    var hint = ''
    if (param.hint != undefined)
      hint = param.hint;

    var id = 'server-edit-dialog-param-form-' + index;
    param.id = id;
    s += '<div class="form-group">';
    if (inputStyle == 'text') {
      s += makeTextInput(id, label, defaultValue, hint);
    } else if (inputStyle == 'checkBox') {
      s += makeCheckboxInput(id, label, hint);
    } else {
      hatoholErrorMsgBox("[Malformed reply] unknown input style: " +
                         inputStyle);
      return '';
    }
    s += '</div>'
    return s;
  }

  function makeTextInput(id, label, defaultValue, hint) {
    s = '';
    s += '  <label for="' + id  + '" class="col-sm-3 control-label">'
    s += gettext(label)
    s += '  </label>';
    s += '  <div class="col-sm-9">';
    s += '    <input type="text" class="form-control" id="' + id +
           '" placeholder="' + hint + '" value="' + defaultValue + '">';
    s += '  </div>'
    return s;
  }

  function makeCheckboxInput(id, label) {
    s = '';
    s += '  <div class="col-sm-offset-3 class="col-sm-9">';
    s += '    <div class="checkbox">';
    s += '    <label>';
    s += '      <input type="checkbox" id="' + id + '">' + gettext(label)
    s += '    </label>';
    s += '    </div>'
    s += '  </div>'
    return s;
  }
};

HatoholServerEditDialogParameterized.prototype.setApplyButtonState = function(state) {
  var btn = $(".ui-dialog-buttonpane").find("button:contains(" +
              this.applyButtonTitle + ")");
  if (state) {
     btn.removeAttr("disabled");
     btn.removeClass("ui-state-disabled");
  } else {
     btn.attr("disabled", "disable");
     btn.addClass("ui-state-disabled");
  }
};

HatoholServerEditDialogParameterized.prototype.fixupApplyButtonState = function(enable) {
  var self = this;

  if (!self.currParamObj) {
    self.setApplyButtonState(false);
    return;
  }
  var paramObj = self.currParamObj;

  var i = 0;
  for (i = 0; i < paramObj.length; i++) {
     var param = paramObj[i];
     if (param.allowEmpty)
       continue;
     if (!$('#' + param.id).val())
       break;
  }
  var state = (i == paramObj.length);
  self.setApplyButtonState(state);
};

HatoholServerEditDialogParameterized.prototype.setServer = function(server) {
  this.server = server;
  // TODO: implement
  this.fixupApplyButtonState();
};
