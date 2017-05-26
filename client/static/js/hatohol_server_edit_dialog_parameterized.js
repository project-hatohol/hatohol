/*
 * Copyright (C) 2013-2014 Project Hatohol
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

var HatoholServerEditDialogParameterized = function(params) {
  var self = this;

  self.operator = params.operator;
  self.server = params.targetServer;
  self.succeededCallback = params.succeededCallback;
  self.windowTitle =
    self.server ? gettext("EDIT MONITORING SERVER") : gettext("ADD MONITORING SERVER");
  self.applyButtonTitle = self.server ? gettext("APPLY") : gettext("ADD");

  var dialogButtons = [{
    text: self.applyButtonTitle,
    click: addButtonClickedCb
  }, {
    text: gettext("CANCEL"),
    click: cancelButtonClickedCb
  }];

  // call the constructor of the super class
  var dialogAttrs = {
    width: 768,
    position: ['center', 50]
  };
  HatoholDialog.apply(
      this, ["server-edit-dialog", self.windowTitle,
             dialogButtons, dialogAttrs]);

  //
  // Dialog button handlers
  //
  function addButtonClickedCb() {
    if (self.server)
      hatoholInfoMsgBox(gettext("Now updating the server ..."), {buttons: []});
    else
      hatoholInfoMsgBox(gettext("Now adding a server..."), {buttons: []});
    postAddServer();
  }

  function cancelButtonClickedCb() {
    self.closeDialog();
  }

  function isHAPI2(type) {
    return isNaN(type) || type == hatohol.MONITORING_SYSTEM_HAPI2;
  }

  function makeQueryData() {
    var paramObj = self.currParamObj;
    var type = $("#selectServerType").val();
    var queryData;

    if (isHAPI2(type)) {
      queryData = {
        'type': hatohol.MONITORING_SYSTEM_HAPI2,
        'uuid': type,
      };
    } else {
      queryData = {
        'type': type,
      };
    }

    for (var i = 0; i < paramObj.length; i++) {
      var param = paramObj[i];

      var val;
      if (param.hidden)
        val = param.default;
      else if (param.inputStyle == 'checkBox')
        val = $('#' + param.elementId).prop('checked');
      else
        val = $('#' + param.elementId).val();

      queryData[param.id] = val;
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
    var type;
    var serverTypes = reply.serverType;

    if (!(serverTypes instanceof Array)) {
      hatoholErrorMsgBox("[Malformed reply] Not found array: serverType");
      return;
    }
    sortByPriority(serverTypes);
    self.paramArray = [];
    self.uuidArray = [];
    for (var i = 0; i < serverTypes.length; i ++) {
      var serverTypeInfo = serverTypes[i];
      var name = serverTypeInfo.name;
      if (!name) {
        hatoholErrorMsgBox("[Malformed reply] Not found element: name");
        return;
      }

      type = serverTypeInfo.type;
      if (type === hatohol.MONITORING_SYSTEM_HAPI2)
        type = getServerTypeId(serverTypeInfo);
      if (type === undefined) {
        hatoholErrorMsgBox("[Malformed reply] Not found element: type");
        return;
      }

      var parameters = serverTypeInfo.parameters;
      if (parameters === undefined) {
        hatoholErrorMsgBox("[Malformed reply] Not found element: parameters");
        return;
      }

      $('#selectServerType').append($('<option>').html(name).val(type));
      self.paramArray[type] = parameters;
      self.uuidArray[type] = serverTypeInfo.uuid;
    }
    self.fixupApplyButtonState();

    if (self.server) {
      var selectElem = $("#selectServerType");
      type = self.server.type;
      if (type == hatohol.MONITORING_SYSTEM_HAPI2)
        type = self.server.uuid;
      selectElem.val(type);
      selectElem.change();
    }
  }

  function makeMainDiv() {
    var mainDiv = $('<div id="add-server-div">');
    var form = $('<form class="form-inline">').appendTo(mainDiv);
    form.append($('<label>').text(gettext('Monitoring server type')));
    var select = $('<select id="selectServerType">').appendTo(form);
    select.append($('<option>').html(gettext('Please select')).val('_header'));
    mainDiv.append('<form id="add-server-param-form" ' +
                   'class="form-horizontal" role="form" autocomplete="off">');
    return mainDiv;
  }

  function sortByPriority(serverTypes) {
    for (var i = 0; i < serverTypes.length; i++) {
      addPriority(serverTypes[i]);
    }
    sortObjectArray(serverTypes, 'priority');
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
    var elem;
    paramObj = JSON.parse(parameters);
    if (!(paramObj instanceof Array)) {
        hatoholErrorMsgBox("[Malformed reply] parameters is not array");
        return;
    }

    for (var i = 0; i < paramObj.length; i++) {
      elem = makeFormOfOneParameter(paramObj[i], i);
      if (!elem)
        continue;
      $('#add-server-param-form').append(elem);
    }

    // Fill value for update
    if (self.server) {
      for (i = 0; i < paramObj.length; i++) {
        var param = paramObj[i];
        if (!(param.id in self.server))
          continue;

        elem = $('#' + paramObj[i].elementId);
        var value = self.server[param.id];
        if (param.inputStyle == 'checkBox')
          elem.prop('checked', value);
        else
          elem.val(value);
      }
    }

    // set events to fix up state of 'apply' button
    for (i = 0; i < paramObj.length; i++) {
      $('#' + paramObj[i].elementId).keyup(function() {
        self.fixupApplyButtonState();
      });
    }
    return paramObj;
  }

  function makeFormOfOneParameter(param, index) {
    if (param.hidden)
      return null;

    var label;
    if (param.label)
      label = param.label;
    else
      label = param.id;

    var defaultValue = '';
    if (param.default !== undefined)
      defaultValue = param.default;

    var inputStyle = param.inputStyle;
    if (inputStyle === undefined)
      inputStyle = 'text';

    var hint = '';
    if (param.hint !== undefined)
      hint = param.hint;

    var elementId = 'server-edit-dialog-param-form-' + index;
    param.elementId = elementId;
    var div = $('<div class="form-group">');
    if (inputStyle == 'text' || inputStyle == 'password') {
      div.append(makeLineInput(elementId, label, defaultValue, hint, inputStyle));
    } else if (inputStyle == 'checkBox') {
      div.append(makeCheckboxInput(elementId, label, hint));
    } else {
      hatoholErrorMsgBox("[Malformed reply] unknown input style: " +
                         inputStyle);
    }
    return div;
  }

  function makeLineInput(id, label, defaultValue, hint, type) {
    var elem = $('<div>');
    var labelEl = $('<label for="' + id  + '" class="col-sm-3 control-label">');
    labelEl.appendTo(elem);
    labelEl.text(gettext(label));

    var div = $('<div class="col-sm-9">').appendTo(elem);
    var input = $('<input type="' + type + '" class="form-control" id="' + id + '">');
    input.appendTo(div);
    input.attr('placeholder', hint);
    input.val(defaultValue);

    return elem;
  }

  function makeCheckboxInput(id, label) {
    var div = $('<div class="col-sm-offset-3 class="col-sm-9">');
    var divCheckbox = $('<div class="checkbox">').appendTo(div);
    var labelEl = $('<label>').appendTo(divCheckbox);
    labelEl.text(gettext(label));
    $('<input type="checkbox" id="' + id + '">').appendTo(labelEl);
    return div;
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
     if (param.hidden)
       continue;
     if (!$('#' + param.elementId).val())
       break;
  }
  var state = (i == paramObj.length);
  self.setApplyButtonState(state);
};
