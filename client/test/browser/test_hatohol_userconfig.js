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

describe('HatoholUserConfig', function() {

  function defaultConnectErrorCallback(XMLHttpRequest, textStatus, errorThrown) {
    expect().fail(function(){
      return "status " + XMLHttpRequest.status +
             ": " + textStatus + ": " + errorThrown });
    // done() may not be unnecessary because the above fail()
    // throw an exception
  }

  it('try to get nonexisting item', function(done) {
    //setLoginDialogCallback();
    var params = {
      itemNames: ['foo'],
      successCallback: function(values) {
        expect(values).not.to.be(undefined);
        expect(values).to.be.an('object');
        expect(values).to.have.key('foo');
        expect(values['foo']).to.be(null);
        done();
      },
      connectErrorCallback: defaultConnectErrorCallback,
    }
    userconfig = new HatoholUserConfig();
    userconfig.get(params);
  });

  it('store a string', function(done) {
    var params = {
      items: {'color':'red and blue'},
      successCallback: function(reply) { done(); },
      connectErrorCallback: defaultConnectErrorCallback,
    }
    userconfig = new HatoholUserConfig();
    userconfig.store(params);
  });

  it('store an integer', function(done) {
    var params = {
      items: {'level': 99},
      successCallback: function(reply) { done(); },
      connectErrorCallback: defaultConnectErrorCallback,
    }
    userconfig = new HatoholUserConfig();
    userconfig.store(params);
  });

  it('store an float', function(done) {
    var params = {
      items: {'height': 18.2},
      successCallback: function(reply) { done(); },
      connectErrorCallback: defaultConnectErrorCallback,
    }
    userconfig = new HatoholUserConfig();
    userconfig.store(params);
  });
});
