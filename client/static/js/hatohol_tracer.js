/*
 * Copyright (C) 2016 Project Hatohol
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

var HatoholTracePoint = {
  PRE_HREF_CHANGE: 0
};

var HatoholTracer = function() {
  var self = this;

  self.listeners = {};
  for (var key in HatoholTracePoint) {
    var tracePointId = HatoholTracePoint[key];
    self.listeners[tracePointId] = [];
  }

  this.addListener = function(tracePointId, func) {
    if (!(tracePointId in self.listeners))
      throw "Unknown tracePointId: " + tracePointId;
    self.listeners[tracePointId].push(func);
  };

  this.pass = function(tracePointId, params) {
    if (!(tracePointId in self.listeners))
      throw "Unknown tracePointId: " + tracePointId;
    var listeners = self.listeners[tracePointId];
    for (var i = 0; i < listeners.length; i++)
        listeners[i](params);
  };
};

var hatoholTracer = new HatoholTracer();
