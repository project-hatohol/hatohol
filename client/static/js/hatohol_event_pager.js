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

var HatoholEventPager = function(params) {
  //
  // Possible parameters in the "params" argument:
  //
  //   numRecordsPerPage: <number> [optional]
  //     Defaut: 50
  //
  //   numTotalRecords: <number> [optional]
  //     Defaut: -1 (unknown)
  //
  //   currentPage: <number> [optional]
  //     Defaut: 0
  //
  //   maxPagesToShow: <number> [optional]
  //     Defaut: 10
  //
  //   selectPageCallback: <function> [optional]
  //     function(page)
  //     Note: Called when a page is selected or numRecordsPerPage is changed.
  //           The "page" argument will be undefined when the numRecordsPerPage
  //           is changed.
  //
  //   parentElements: <jQuery object> [optional]
  //     Default: $("ul.pagination").
  //     Note: Elements to show pager
  //
  //   numRecordsPerPageEntries: <jQuery object> [optional]
  //     Default: $("input.num-records-per-page");
  //     Note: input elements to change numRecordsPerPage
  //
  self = this;
  self.parentElements = $("ul.pagination");
  self.numRecordsPerPageEntries = $("input.num-events-per-page");
  self.numRecordsPerPage = 50;
  self.currentPage = 0;
  self.numberOfEvents = -1; // unknown
  self.selectPageCallback = null;

  self.update(params);

  $(self.numRecordsPerPageEntries).change(function() {
    var val = parseInt(self.numRecordsPerPageEntries.val());
    if (!isFinite(val) || val < 0)
      val = self.numRecordsPerPage;
    self.numRecordsPerPageEntries.val(val);
    self.numRecordsPerPage = val;
    if (self.selectPageCallback)
      self.selectPageCallback();
  });
};

HatoholEventPager.prototype.applyParams = function(params) {
  if (params && params.parentElements)
    this.parentElements = params.parentElements;
  if (params && params.numRecordsPerPageEntries)
    this.numRecordsPerPageEntries = params.numRecordsPerPageEntries;
  if (params && !isNaN(params.numRecordsPerPage))
    this.numRecordsPerPage = params.numRecordsPerPage;
  if (params && !isNaN(params.currentPage))
    this.currentPage = params.currentPage;
  if (params && !isNaN(params.numberOfEvents))
    this.numberOfEvents = params.numberOfEvents;
  if (params && params.selectPageCallback)
    this.selectPageCallback = params.selectPageCallback;
};

HatoholEventPager.prototype.update = function(params) {
  this.applyParams(params);

  var self = this;
  var parent = this.parentElements;
  var entrieRecords = this.numberOfEvents;
  var createItem = function(label, enable, getPageFunc) {
    var anchor = $("<a>", {
      href: "#",
      html: label,
      click: function () {
	var page;
	if (getPageFunc)
	  page = getPageFunc();
	else
	  page = parseInt($(this).text()) - 1;
	if (!enable)
	  return;
	if (self.selectPageCallback)
	  self.selectPageCallback(page);
	self.currentPage = page;
	self.update();
      }
    });
    var item = $("<li/>").append(anchor);
    if (!enable)
      item.addClass("disabled");
    return item;
  };
  var i, item, enable;

  parent.empty();

  enable = this.currentPage > 4;
  parent.append(createItem('<i class="glyphicon glyphicon-backward"></i>', enable, function() {
    return self.currentPage - 5;
  }));

  enable = this.currentPage > 0;
  parent.append(createItem('<i class="glyphicon glyphicon-chevron-left"></i>', enable, function() {
    return self.currentPage - 1;
  }));

  enable = true;
  parent.append(createItem(self.currentPage + 1, enable, function() {
    return self.currentPage;
  }).addClass("active"));

  enable = entrieRecords == this.numRecordsPerPage;
  parent.append(createItem('<i class="glyphicon glyphicon-chevron-right"></i>', enable, function() {
    return self.currentPage + 1;
  }));

  enable = entrieRecords == this.numRecordsPerPage;
  parent.append(createItem('<i class="glyphicon glyphicon-forward"></i>', enable, function() {
    return self.currentPage + 5;
  }));

  $(self.numRecordsPerPageEntries).val(self.numRecordsPerPage);
};
