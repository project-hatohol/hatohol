/*
 * Copyright (C) 2014 - 2015 Project Hatohol
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

var HatoholTimeRangeSelector = function(options) {
  var self = this;
  var secondsInHour = 60 * 60;

  self.options = options || {};
  self.id = self.options.id || "hatohol-time-range-selector";
  self.timeRange = getTimeRange();
  self.settingTimeRange = false;
  self.slider = undefined;

  function getTimeRange() {
    var timeRange = {
      begin: undefined,
      end: undefined,
      minSpan: secondsInHour,
      maxSpan: secondsInHour * 24,
      min: undefined,
      max: undefined,
      set: function(beginTime, endTime) {
        this.setEndTime(endTime);
        this.begin = beginTime;
        if (this.getSpan() > this.maxSpan)
          this.begin = this.end - this.maxSpan;
        if (this.getSpan() < this.minSpan) {
          if (this.begin + this.minSpan >= this.max) {
            this.end = this.max;
            this.begin = this.max - this.minSpan;
          } else {
            this.end = this.begin + this.minSpan;
          }
        }
      },
      setEndTime: function(endTime) {
        var span = this.getSpan();
        this.end = endTime;
        this.begin = this.end - span;
        if (!this.max || this.end > this.max) {
          this.max = this.end;
          this._adjustMin();
        }
      },
      _adjustMin: function() {
        var date;
        // 1 week ago
        this.min = this.max - secondsInHour * 24 * 7;
        // Adjust to 00:00:00
        date = getDate(this.min * 1000);
        this.min -=
          date.getHours() * 3600 +
          date.getMinutes() * 60 +
          date.getSeconds();
      },
      getSpan: function() {
        if (this.begin && this.end)
          return this.end - this.begin;
        else
          return secondsInHour * 6;
      }
    };
    return timeRange;
  }

  function getDate(timeMSec) {
    var plotOptions = {
      mode: "time",
      timezone: "browser"
    };
    return $.plot.dateGenerator(timeMSec, plotOptions);
  }
};

HatoholTimeRangeSelector.prototype.draw = function() {
  var self = this;
  var secondsInHour = 60 * 60;
  var timeRange = self.timeRange;

  self.slider = $("#" + self.id).slider({
    range: true,
    min: timeRange.min,
    max: timeRange.max,
    values: [timeRange.begin, timeRange.end],
    change: function(event, ui) {
      if (self.settingTimeRange)
        return;

      self.setTimeRange(ui.values[0], ui.values[1]);

      if (self.options.setTimeRangeCallback)
        self.options.setTimeRangeCallback(timeRange.begin, timeRange.end);
    },
    slide: function(event, ui) {
      var beginTime = ui.values[0], endTime = ui.values[1];

      if (timeRange.begin != ui.values[0])
        endTime = ui.values[0] + timeRange.getSpan();
      if (endTime > timeRange.max)
	endTime = timeRange.max;
      if (endTime - ui.values[0] < timeRange.minSpan)
        beginTime = endTime - timeRange.minSpan;
      self.setTimeRange(beginTime, endTime);
    }
  });
  self.slider.slider('pips', {
    rest: 'label',
    last: false,
    step: secondsInHour * 12,
    formatLabel: function(val) {
      var now = new Date();
      var date = new Date(val * 1000);
      var dayLabel = {
        0: gettext("Sun"),
        1: gettext("Mon"),
        2: gettext("Tue"),
        3: gettext("Wed"),
        4: gettext("Thu"),
        5: gettext("Fri"),
        6: gettext("Sat")
      };
      if (date.getHours() === 0) {
        if (now.getTime() - date.getTime() > secondsInHour * 24 * 7 * 1000)
          return formatDate(val);
        else
          return dayLabel[date.getDay()];
      } else {
        return "";
      }
    }
  });
  self.slider.slider('float', {
    formatLabel: function(val) {
      return formatDate(val);
    }
  });
};

HatoholTimeRangeSelector.prototype.setTimeRange = function(minSec, maxSec) {
  var self = this;
  var values;

  if (self.settingTimeRange)
    return;

  self.timeRange.set(minSec, maxSec);

  if (!self.slider)
    return;

  self.settingTimeRange = true;
  values = $("#" + self.id).slider("values");
  if (self.timeRange.begin != values[0])
    $("#" + self.id).slider("values", 0, self.timeRange.begin);
  if (self.timeRange.end != values[1])
    $("#" + self.id).slider("values", 1, self.timeRange.end);
  self.settingTimeRange = false;
};

HatoholTimeRangeSelector.prototype.getTimeSpan = function() {
  return this.timeRange.getSpan();
};

HatoholTimeRangeSelector.prototype.getBeginTime = function() {
  return this.timeRange.begin;
};

HatoholTimeRangeSelector.prototype.getEndTime = function() {
  return this.timeRange.end;
};
