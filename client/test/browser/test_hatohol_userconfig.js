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

  function storeOneItem(done, item, successCallback) {

    if (!successCallback)
        successCallback = function(reply) { done(); }
  
    var params = {
      items: item,
      successCallback: successCallback,
      connectErrorCallback: defaultConnectErrorCallback,
    }
    var userconfig = new HatoholUserConfig();
    userconfig.store(params);
  }

  function storeAndGetOneItem(done, item) {
    storeOneItem(done, item, function(reply) {
      var itemNames = new Array();
      for (name in item)
        itemNames.push(name);
      var params = {
        itemNames: itemNames,
        successCallback: function(obtained) {
          expect(obtained).to.eql(item);
          done();
        },
        connectErrorCallback: defaultConnectErrorCallback,
      }
      var userconfig = new HatoholUserConfig();
      userconfig.get(params);
    });
  }

  //
  // Test cases
  //

  before(function(done) {
    // TODO: make sure to create a session ID for the test.
    //       currently the former tests create it. So no problem happens w/o it.
    done();
  });

  beforeEach(function(done) {
    var params = {
      url: '/test/delete_user_config',
      pathPrefix: '',
      replyCallback: function(reply, parser) { done(); },
      connectErrorCallback: defaultConnectErrorCallback,
      replyParser: getInactionParser(),
    };
    new HatoholConnector(params);
  });

  it('try to get nonexisting item', function(done) {
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
    storeOneItem(done, {'color':'red and blue'});
  });

  it('store an integer', function(done) {
    storeOneItem(done, {'level': 99});
  });

  it('store an float', function(done) {
    storeOneItem(done, {'height': 18.2});
  });

  it('store a boolean', function(done) {
    storeOneItem(done, {'beautiful': true});
  });

  it('store null', function(done) {
    storeOneItem(done, {'nurunuru': null});
  });

  it('store and get a string', function(done) {
    storeAndGetOneItem(done, {'color':'red and blue'});
  });

  it('store and get an integer', function(done) {
    storeAndGetOneItem(done, {'cow': 15});
  });

  it('store and get a float', function(done) {
    storeAndGetOneItem(done, {'height': 18.9});
  });

});
