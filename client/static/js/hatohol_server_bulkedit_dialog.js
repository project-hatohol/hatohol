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

var HatoholMultipleServersEditDialog = function(params) {
  var self = this;

  self.operator = params.operator;
  self.server = params.targetServer;
  self.succeededCallback = params.succeededCallback;
  self.windowTitle = gettext("BULK UPLOAD MONITORING SERVERS");
  self.applyButtonTitle = gettext("APPLY");

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
      this, ["multiple-servers-edit-dialog", self.windowTitle,
             dialogButtons, dialogAttrs]);

  //
  // Dialog button handlers
  //
  function addButtonClickedCb() {
    postAddServers();
  }

  function cancelButtonClickedCb() {
    self.closeDialog();
  }

  function makeQueryData() {
    var paramObj = self.currParamObj;
    var type = $("#selectMultipleServersType").val();
    var queryData = {'type':type, '_extra':{}};
    for (var i = 0; i < paramObj.length; i++) {
      var param = paramObj[i];
      var val;
      var row = self.tsvData[self.batchUpdateIter];

      if (param.hidden || (row[param.id] == '' && param.default))
        val = param.default;
      else if (param.inputStyle == 'checkBox')
        val = row[param.id] != '' && row[param.id] != '0';
      else
        val = row[param.id];

      queryData[param.id] = val;
    }
    return queryData;
  }

  function postAddServers() {
    self.batchUpdateIter = 0;
    self.succeeded = 0;
    self.setApplyButtonState(false);
    postAddServer(0);
  }

  function _postAddServer(idx) {
    var url = "/server";
    var row = self.tsvData[idx];
    var server_id = row['#ID'];
    if (server_id)
      url += "/" + server_id;
    new HatoholConnector({
      url: url,
      request: server_id ? "PUT" : "POST",
      data: makeQueryData(),
      replyCallback: replyCallback,
      parseErrorCallback: errorCallback,
    });
  }

  function postAddServer(idx) {
    if (idx >= self.tsvData.length)
      return 0;
    _postAddServer(idx);
    return 1;
  }

  function setUpdateStatus(idx, flag, title) {
    if (flag === undefined)
      $('#add-multiple-servers-table-' + idx + '-' + (-1)).css('backgroundColor', "");
    else
      $('#add-multiple-servers-table-' + idx + '-' + (-1)).css('backgroundColor', flag ? '#8f8' : '#f88');
    if (title)
      $('#add-multiple-servers-table-' + idx + '-' + (-1)).attr({'title': title});
  }

  function continueAddServer(isSucceeded, msg) {
    var isOk = undefined;

    setUpdateStatus(self.batchUpdateIter, isSucceeded, msg);

    if (isSucceeded)
      self.succeeded++;
    self.batchUpdateIter++;
    if (postAddServer(self.batchUpdateIter))
      return;

    isOk = self.succeeded == self.batchUpdateIter;

    if (self.succeeded) {
      hatoholInfoMsgBox(gettext("Successfully uploaded.") +
                        '(' + self.succeeded + '/'+ self.batchUpdateIter + ')');
      if (isOk)
        self.closeDialog();
      if (self.succeededCallback)
        self.succeededCallback();
    } else {
      hatoholInfoMsgBox(gettext("Failed to uploaded."));
    }
  }

  function replyCallback(reply, parser) {
    continueAddServer(true);
  }

  function errorCallback(reply, parser) {
    var msg = parser.getMessage();
    if (!msg)
      msg = gettext("Failed to parse the received packet.");
    msg += parser.optionMessages;

    continueAddServer(false, msg);
  }
};

HatoholMultipleServersEditDialog.prototype = Object.create(HatoholDialog.prototype);
HatoholMultipleServersEditDialog.prototype.constructor = HatoholMultipleServersEditDialog;

HatoholMultipleServersEditDialog.prototype.createMainElement = function() {
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

      $('#selectMultipleServersType').append($('<option>').html(name).val(type));
      self.paramArray[type] = parameters;
    }
    self.fixupApplyButtonState();

    if (self.server) {
      var selectElem = $("#selectMultipleServerType");
      selectElem.val(self.server.type);
      selectElem.change();
    }
  }

  function makeMainDiv() {
    var mainDiv = $('<div id="add-multiple-servers-div">');
    var form = $('<form class="form-inline">').appendTo(mainDiv);

    div = $('<div>');
    div.append($('<label>').text(gettext('Monitoring server type')));
    var select = $('<select id="selectMultipleServersType">').appendTo(form);
    select.append($('<option>').html(gettext('Please select')).val('_header'));
    div.append(select);
    form.append(div);

    var div = $('<div>');
    div.append($('<label>').text(gettext('TSV File')));
    div.append($('<input id="optionMultipleServersTSVFile" type="file" disabled class="ui-state-disabled">'));
    form.append(div);

    //mainDiv.append('<form id="add-multiple-servers-param-form" class="form-horizontal" role="form" autocomplete="off">');
    mainDiv.append('<table class="table" id="add-multiple-servers-table" border="1">');
    return mainDiv;
  }
};

HatoholMultipleServersEditDialog.prototype.onAppendMainElement = function () {
  var self = this;

  self.fixupApplyButtonState();

  $('#selectMultipleServersType').change(function() {
    var optionFile = $('#optionMultipleServersTSVFile');

    self.currParamObj = undefined;
    optionFile.val([]);
    $('#add-multiple-servers-table').empty();

    var type = $("#selectMultipleServersType").val();
    if (type != "_header") {
      var paramObj = setupMultipleServersTable(self.paramArray[type]);
      self.currParamObj = paramObj;
      optionFile.removeAttr("disabled");
      optionFile.removeClass("ui-state-disabled");
    } else {
      optionFile.attr("disabled");
      optionFile.addClass("ui-state-disabled");
    }
    self.fixupApplyButtonState();
  });

  $('#optionMultipleServersTSVFile').change(function() {
    var f = this.files[0];

    $('#selectMultipleServersTableBody').remove();

    var fileReader = new FileReader();
    fileReader.readAsText(f, 'utf-8');
    fileReader.onload = parseTSVFile;
  });

  function parseTSVFile(evt) {
    var fileString = evt.target.result;
    var lines = fileString.split(/\r\n|\r|\n/);
    var colnames = null;
    var obj = {};
    var row;
    var type = $('#selectMultipleServersType').val();
    var parameters = type != '_header' ? self.paramArray[type] : null;
    var paramObj = parameters ? JSON.parse(parameters) : null;
    var tbody = $('<tbody id="selectMultipleServersTableBody">');
    var paramHash = {};
    var paramId2col = {};
    var i;
    var badRows = 0;

    if (!paramObj || !(paramObj instanceof Array)) {
      hatoholErrorMsgBox("[Malformed reply] parameters is not array");
      return;
    }

    self.tsvData = [];

    $.each(paramObj, function(idx, val) {
             paramId2col[val.id] = -1;
             paramHash[val.id] = val;
           });

    for (var i = 0; i < lines.length; i++) {
      var line = lines[i];
      var tr = $('<tr>');
      var cols;
      var badCols = 0;
      var idBadl

      if (line.length == 0)
        continue;

      cols = lines[i].split(/\t/);

      if (colnames === null) {
        var missingParams;

        if (cols[0] != '#ID')
          continue;
        colnames = cols;

        $.each(cols, function(idx, val) {
                 paramId2col[val] = idx;
               });

        missingParams = $.map(paramId2col,
                              function(val, key) { return val < 0 ? key : null; });
        if (missingParams.length) {
          hatoholErrorMsgBox(gettext('Sredome columns are missing: ') + '\n' +
                             $.map(missingParams, function(val, idx) {
                               return val + '(' + gettext(paramHash[val].label) + ')';
                             }).join(','));
          return;
        }
        continue;
/*
  for (i = 0; i < paramObj.length; i++) {
     var param = paramObj[i];
     if (param.allowEmpty)
       continue;
     if (param.hidden)
       continue;
     if (!$('#' + param.elementId).val())
       break;
  }
*/
      } else if (!colnames || cols[0].match(/^#/))
        continue;

      row = {};
      for (j = -1; j < paramObj.length; j++) {
        var td = $('<td>');
        var param = j >= 0 ? paramObj[j] : null;
        var id = param ? param.id : '#ID';
        var val = cols[paramId2col[id]];
        row[id] = val !== undefined ? val : '';
        var isBad = false;

        if (!param) {
          param = {
            inputStyle: 'text',
            allowEmpty: true,
          };
        }

        if (param.inputStyle == 'hidden')
          continue;

        if (param.inputStyle == 'password')
          td.text("********");
        else {
          td.text("" + val);
          td.attr({ id: 'add-multiple-servers-table-' + self.tsvData.length + '-' + j });
        }

        if (row[id] == '') {
          if (param.default !== undefined)
            val = param.default;
          if (val == '' && !param.allowEmpty)
            isBad = true;
        }

        if (isBad) {
          td.css("backgroundColor", "red");
          badCols++;
        }
        tr.append(td);
      }
      tr.attr({ id: 'add-multiple-servers-table-' + self.tsvData.length, });
      tbody.append(tr);
      if (badCols)
        badRows++;
      self.tsvData.push(row);
    }
    if (!self.tsvData.length) {
      hatoholErrorMsgBox(gettext('No valid rows in TSV.'));
      return;
    }
    $('#add-multiple-servers-table').append(tbody);
    if (badRows) {
      hatoholErrorMsgBox(gettext('Error found in TSV: ' + badRows + ' / ' + self.tsvData.length));
      self.setApplyButtonState(false);
      return;
    }
    self.setApplyButtonState(true);
  }

  function setupMultipleServersTable(parameters) {
    var paramObj = JSON.parse(parameters);
    var thead = $('<thead>');
    var tr = $('<tr>');

    $('#add-multiple-servers-table').empty();
    self.setApplyButtonState(false);

    if (!(paramObj instanceof Array)) {
      hatoholErrorMsgBox("[Malformed reply] parameters is not array");
      return;
    }

    for (var i = -1; i < paramObj.length; i++) {
      var id;
      var label = null;
      var hint = null;
      var title = '';

      if (i < 0) {
        id = '#ID';
      } else {
        var param = paramObj[i];
        if (param.hidden)
          continue;
        id = param.id;
        label = param.label;
        title = $.map(param, function(val, key) {
                        return key != 'label' ? key + ': ' + val : null;
                      }).join('\n');
      }
      if (id === undefined)
        continue;
      if (label === null)
        label = id;

      tr.append($('<th>').text(gettext(label)).attr({'title': title}));
    }

    thead.append(tr);

    $('#add-multiple-servers-table').append(thead);

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
    if (param.default != undefined)
      defaultValue = param.default

    var inputStyle = param.inputStyle;
    if (inputStyle == undefined)
      inputStyle = 'text';

    var hint = ''
    if (param.hint != undefined)
      hint = param.hint;

    var elementId = 'multiple-servers-edit-dialog-param-form-' + index;
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
    var input = $('<input type="checkbox" id="' + id + '">').appendTo(labelEl);
    return div;
  }
};

HatoholMultipleServersEditDialog.prototype.setApplyButtonState = function(state) {
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

HatoholMultipleServersEditDialog.prototype.fixupApplyButtonState = function(enable) {
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
