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
  var self = this;
};

HatoholUserConfig.prototype.get = function(params) {
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
  this.connector = new HatoholConnector({
    url: '/userconfig',
    data: {'items': params.itemNames},
    pathPrefix: '',
    replyCallback: function(reply, parser) {
      return params.successCallback(reply);
    },
    connectErrorCallback: params.connectErrorCallback,
    replyParser: getInactionParser(),
  });
}

HatoholUserConfig.prototype.store = function(params) {
  //
  // params has the following parameters.
  //
  // items: <object> [mandatory]
  //   items to be stored.
  //
  // successCallback: <function> [optional]
  //   A callback function that is called on the success.
  //   The function form: func(returnedBody),
  //
  // connectErrorCallback: <function> [mandatory]
  //   function(XMLHttpRequest, textStatus, errorThrown)
  //   A callback function that is called on the error.
  //
  new HatoholConnector({
    url: '/userconfig',
    request: 'POST',
    data: JSON.stringify(params.items),
    pathPrefix: '',
    contentType: 'application/json',
    replyCallback: function(reply, parser) {
      if (params.successCallback)
          params.successCallback(reply);
    },
    connectErrorCallback: params.connectErrorCallback,
    replyParser: getInactionParser(),
  });
}

HatoholUserConfig.prototype.findOrDefault = function(obj, confName, defaultVal) {
  if (!(confName in obj))
    return defaultVal;

  var confVal = obj.confName;
  if (typeof confVal !== typeof defaultVal)
    return defaultVal;
  return confVal;
}
