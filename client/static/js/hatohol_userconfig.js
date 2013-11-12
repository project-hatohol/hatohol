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

var HatoholUserConfig = function(params) {
  //
  // params has the following parameters.
  //
  // itemNames: <array> [mandatory]
  //   item names for values to be obtained.
  //
  // successCallback: <function> [mandatory]
  //   A callback function that is called on the success.
  //   The function form: func(returnedObj),
  //   where 'returnedObj' is an object such as
  //   '{'a':1, 'book':'foo', 'X':-5}, where 'a', 'book', and 'X' are
  //   item names specified in itemaNames.
  //   If the value for the specified item hasn't been stored yet,
  //   the default value is null.
  //
  // connectErrorCallback: <function> [mandatory]
  //   function(XMLHttpRequest, textStatus, errorThrown)
  //   A callback function that is called on the error.
  //
  var self = this;
  self.itemIndex = 0
  self.values = {}
  mainIterate();

  function mainIterate() {
    if (self.itemIndex >= params.itemNames.length) {
      params.successCallback(self.values);
      return;
    }
    new HatoholConnector({
      url: '/userconfig/' + encodeURI(params.itemNames[self.itemIndex]),
      pathPrefix: '',
      replyCallback: function(reply, parser) {
        var itemName = params.itemNames[self.itemIndex];
        self.values[itemName] = reply.itemName;
        self.itemIndex++;
        mainIterate();
      },
      connectErrorCallback: params.connectErrorCallback,
      replyParser: function(data) {
        return { getStatus: function() { return REPLY_STATUS.OK; } }
      },
    });
  }
};
