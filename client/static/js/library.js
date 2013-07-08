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

Array.prototype.uniq = function() {
  var o = {}, i, len = this.length, r = [];
  for (i = 0; i < len; ++i) o[this[i]] = true;
  for (i in o) r.push(i);
  return r;
};

function padDigit(val, len) {
  s = "00000000" + val;
  return s.substr(-len);
}

function formatDate(str) {
  var val = new Date();
  val.setTime(Number(str) * 1000);
  var d = val.getFullYear() + "/" + padDigit(val.getMonth() + 1, 2) + "/" + padDigit(val.getDate(), 2);
  var t = padDigit(val.getHours(), 2) + ":" + padDigit(val.getMinutes(), 2) + ":" + padDigit(val.getSeconds(), 2);
  return d + " " + t;
}

function formatSecond(sec) {
  var t = padDigit(parseInt(sec / 3600), 2) + ":" + padDigit(parseInt(sec / 60) % 60, 2) + ":" + padDigit(sec % 60, 2);
  return t;
}

function setCandidate(target, list) {
  var x;
  var s;

  for (x = 0; x < list.length; ++x) {
    s += "<option>" + list[x] + "</option>";
  }
  target.append(s);
}

function buildChooser() {
  var targets = [
    "#select-server",
    "#select-group",
    "#select-host",
    "#select-application",
  ];

  var klass = "";
  var x;
  var s;

  for (x = 0; x < targets.length; ++x) {
    s = $(targets[x]).val();
    if ( "undefined" != typeof s && "---------" != s ) {
      klass = klass + "." + s;
    }
  }

  return klass;
}

function chooseRow() {
  var klass = buildChooser();

  $("#table tbody tr").css("display", "");
  if ( "" != klass ) {
    $("#table tbody tr:not(" + klass + ")").css("display", "none");
  }
}

function chooseColumn() {
  var klass = buildChooser();

  $("#table td").css("display", "");
  if ( "" != klass ) {
    $("#table td:not(" + klass + ")").css("display", "none");
  }
}

function setStatus(value) {
  var elem;
  var x;
  var s;

  if ( "class" in value ) {
    $("#sts button").attr("class", "btn dropdown-toggle btn-" + value["class"]);
  }

  if ( "label" in value ) {
    elem = $("#sts button span:first");
    elem.empty();
    elem.append(value["label"]);
  }

  if ( "lines" in value ) {
    s = "";
    for (x = 0; x < value["lines"].length; ++x) {
      s += "<li>" + value["lines"][x] + "</li>";
    }

    elem = $("#sts ul");
    elem.empty();
    elem.append(s);
  }
}

function update(param) {
  setStatus({
    "class" : "warning",
    "label" : "DRAW",
    "lines" : [ "描画中" ],
  });

  updateCore(param);

  setStatus({
    "class" : "success",
    "label" : "DONE",
    "lines" : [],
  });
}

function schedule(timer, table, param) {
  setTimeout(function() {
    setStatus({
      "class" : "warning",
      "label" : "LOAD",
      "lines" : [ "バックエンドと通信中" ],
    });

    $.getJSON("/tunnel/" + table + ".json", function(json) {
      rawData = json;
      update(param);
    });
  }, timer);
}
