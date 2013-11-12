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

  it('try to get nonexisting item', function(done) {
    //setLoginDialogCallback();
    var params = {
      itemNames: ['foo'],
      successCallback: function(values) {
        expect(values).to.be.an('object');
        expect(values).to.have.key('foo');
        done();
      },
      connectErrorCallback: function(XMLHttpRequest, textStatus, errorThrown) {
        expect().fail(textStatus);
        done();
      }
    }
    new HatoholUserConfig(params);
  });
});
