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

// ---------------------------------------------------------------------------
// HatoholDeleter
// ---------------------------------------------------------------------------
var HatoholDeleter = function(deleteParameters) {
  //
  // deleteParameters has following parameters.
  //
  // * id
  // * type
  // * deletedIdArray
  // TODO: Add the description.
  //
  var self = this;
  self.start(deleteParameters);
}

HatoholDeleter.prototype.start = function(deleteParameters) {
  new HatoholConnector({
    url: '/' + deleteParameters.type + '/' + deleteParameters.id,
      request: "DELETE",
      context: deleteParameters.deletedIdArray,
      replyCallback: function(data, parser, context) {
        parseDeleteResult(data, context);
      },
      connectErrorCallback: function(XMLHttpRequest,
                              textStatus, errorThrown) {
        var errorMsg = "Error: " + XMLHttpRequest.status + ": " +
        XMLHttpRequest.statusText;
        hatoholErrorMsgBox(errorMsg);
        deleteParameters.deleteIdArray.errors++;
      },
      completionCallback: function(context) {
        compleOneDel(context);
      }
  });

  function checkParseResult(data) {
    var msg;
    var malformed =false;
    if (data.result == undefined)
      malformed = true;
    if (!malformed && !data.result && data.message == undefined)
      malformed = true;
    if (malformed) {
      msg = "The returned content is malformed: " +
        "Not found 'result' or 'message'.\n" +
        JSON.stringify(data);
      hatoholErrorMsgBox(msg);
      return false;
    }
    if (!data.result) {
      msg = "Failed:\n" + data.message;
      hatoholErrorMsgBox(msg);
      return false;
    }
    if (data.id == undefined) {
      msg = "The returned content is malformed: " +
        "'result' is true, however, 'id' missing.\n" +
        JSON.stringfy(data);
      hatoholErrorMsgBox(msg);
      return false;
    }
    return true;
  }

  function parseDeleteResult(data, deletedIdArray) {
    if (!checkParseResult(data))
      return;
    if (!(data.id in deletedIdArray)) {
      alert("Fatal Error: You should reload page.\nID: " + data.id +
          " is not in deletedIdArray: " + deletedIdArray);
      deletedIdArray.errors++;
      return;
    }
    delete deletedIdArray[data.id];
  }

  function compleOneDel(deletedIdArray) {
    deletedIdArray.count--;
    var completed = deletedIdArray.total - deletedIdArray.count;
    hatoholErrorMsgBox(gettext("Deleting...") + " " + completed +
                       " / " + deletedIdArray.total);
    if (deletedIdArray.count > 0)
      return;

    // close dialogs
    hatoholInfoMsgBox(gettext("Completed. (Number of errors: ") +
                      deletedIdArray.errors + ")");

    // update the main view
    startConnection(deleteParameters.type, updateCore);
  }
}
