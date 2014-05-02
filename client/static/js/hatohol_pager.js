/*
 * Copyright (C) 2014 Project Hatohol
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

var HatoholPager = function(params) {
  this.parentElements = $("ul.pagination");
  this.numRecordsPerPage = 50;
  this.currentPage = 0;
  this.numTotalRecords = 0;
  this.switchPageCallback = null;

  if (params && params.parentElements)
    this.parentElements = params.parentElements;
  if (params && !isNaN(params.numRecordsPerPage))
    this.numRecordsPerPage = params.numRecordsPerPage;
  if (params && !isNaN(params.currentPage))
    this.currentPage = params.currentPage;
  if (params && !isNaN(params.numTotalRecords))
    this.numTotalRecords = params.numTotalRecords;
  if (params && params.switchPageCallback)
    this.switchPageCallback = params.switchPageCallback;

  this.update();
};

HatoholPager.prototype.getTotalNumberOfPages = function() {
  if (this.numTotalRecords < 1)
    return -1;
  return Math.ceil(this.numTotalRecords / this.numRecordsPerPage);
}

HatoholPager.prototype.update = function() {
  var self = this;
  var parent = this.parentElements;
  var numPages = this.getTotalNumberOfPages();
  var createItem = function(label, getPageFunc) {
    var anchor = $("<a>", {
      href: "#",
      html: label,
      click: function () {
	var page;
	if (getPageFunc)
	  page = getPageFunc();
	else
	  page = parseInt($(this).text()) - 1;
	if (page < 0 || (numPages >= 0 && page >= numPages))
	  return;
	if (self.switchPageCallback)
	  self.switchPageCallback(page);
	self.currentPage = page;
	self.update();
      }
    });
    return $("<li/>").append(anchor);
  }
  var item;

  parent.empty();

  for (i = 0; i < numPages; ++i) {
    item = createItem("" + (i + 1));
    if (i == this.currentPage)
      item.addClass("active");
    parent.append(item);
  }

  if (numPages > 1 || numPages < 0) {
    parent.prepend(createItem("&laquo;", function() {
      return self.currentPage - 1;
    }));
    parent.append(createItem("&raquo;", function() {
      return self.currentPage + 1;
    }));
  }
};
