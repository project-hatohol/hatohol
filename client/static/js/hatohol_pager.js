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

  if (params.parentElements)
    this.parentElements = params.parentElements;
  if (!isNaN(params.numRecordsPerPage))
    this.numRecordsPerPage = params.numRecordsPerPage;
  if (!isNaN(params.currentPage))
    this.currentPage = params.currentPage;
  if (!isNaN(params.numTotalRecords))
    this.numTotalRecords = params.numTotalRecords;

  this.update();
};

HatoholPager.prototype.getTotalNumberOfPages = function() {
  if (this.numTotalRecords < 1)
    return -1;
  return Math.ceil(this.numTotalRecords / this.numRecordsPerPage);
}

HatoholPager.prototype.update = function() {
  var title, klass;
  var parent = this.parentElements;
  var numPages = this.getTotalNumberOfPages();

  parent.empty();

  for (i = 0; i < numPages; ++i) {
    title = '<a href="#">' + (i + 1) + '</a>';
    klass = (i == this.currentPage) ? "active" : "";
    $("<li/>", {
      html: title,
      class: klass,
    }).appendTo(parent);
  }
  if (numPages > 1 || numPages < 0) {
    parent.prepend('<li><a href="#">&laquo;</a></li>');
    parent.append('<li><a href="#">&raquo;</a></li>');
  }
};
