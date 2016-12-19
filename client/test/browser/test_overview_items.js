describe('OverviewItems', function() {
  var TEST_FIXTURE_ID = 'overviewItemsFixture';
  var viewHTML;
  var defaultItems = [
    {
      "serverId": 1,
      "id": 1,
      "hostId": "10101",
      "brief": "cpu usage",
      "lastValueTime": 1415232279,
      "lastValue": "54.282349",
      "prevValue": "24.594534",
      "itemGroupName": "group1",
      "unit": "%",
      "valueType": hatohol.ITEM_INFO_VALUE_TYPE_FLOAT,
    },
    {
      "serverId": 2,
      "id": 2,
      "hostId": "10102",
      "brief": "cpu usage",
      "lastValueTime": 1415232586,
      "lastValue": "23.213421",
      "prevValue": "98.645236",
      "itemGroupName": "group2",
      "unit": "%",
      "valueType": hatohol.ITEM_INFO_VALUE_TYPE_FLOAT,
    },
  ];
  var defaultServers = {
    "1": {
      "nickname": "Zabbix1",
      "type": 0,
      "ipAddress": "192.168.1.100",
      "hosts": {
        "10101": {
          "name": "Host1",
        },
      },
    },
    "2": {
      "nickname": "Zabbix2",
      "type": 0,
      "ipAddress": "192.168.1.101",
      "hosts": {
        "10102": {
          "name": "Host2",
        },
      },
    },
  };

  function getOperator() {
    var operator = {
      "userId": 3,
      "name": "foobar",
      "flags": hatohol.ALL_PRIVILEGES
    };
    return new HatoholUserProfile(operator);
  }

  function itemsJson(items, servers) {
    return JSON.stringify({
      apiVersion: hatohol.FACE_REST_API_VERSION,
      errorCode: hatohol.HTERR_OK,
      items: items ? items : [],
      servers: servers ? servers : {}
    });
  }

  function fakeAjax() {
    var requests = this.requests = [];
    this.xhr = sinon.useFakeXMLHttpRequest();
    this.xhr.onCreate = function(xhr) {
      requests.push(xhr);
    };
  }

  function restoreAjax() {
    this.xhr.restore();
  }

  function respond(items) {
    var header = { "Content-Type": "application/json" };
    this.requests[0].respond(200, header, itemsJson());
    this.requests[1].respond(200, header, items);
  }
  
  beforeEach(function(done) {
    var contentId = "main";
    var setupFixture = function() {
      $("#" + TEST_FIXTURE_ID).append($("<div>", { id: contentId }));
      $("#" + contentId).html(viewHTML);
      fakeAjax();
      done();
    };
    var loadFixture = function() {
      $("#" + TEST_FIXTURE_ID).append($("<iframe>", {
        id: "fixtureFrame",
        src: "../../ajax_overview_items?start=false",
        load: function() {
          viewHTML = $("#" + contentId, this.contentDocument).html();
          setupFixture();
        }
      }));
    };

    $('body').append($('<div>', { id: TEST_FIXTURE_ID }));

    if (viewHTML)
      setupFixture();
    else
      loadFixture();
  });

  afterEach(function() {
    restoreAjax();
    $("#" + TEST_FIXTURE_ID).remove();
  });

  it('new with empty data', function() {
    var view = new OverviewItems(getOperator());
    $('button.latest-button').click();
    respond(itemsJson());
    var heads = $('div#' + TEST_FIXTURE_ID + ' h2');
    expect(heads).to.have.length(1);
    expect($('#table')).to.have.length(1);
  });

  it('With items', function() {
    var view = new OverviewItems(getOperator());
    $('button.latest-button').click();
    respond(itemsJson(defaultItems, defaultServers));
    expect($('#table')).to.have.length(1);
    expect($('tr')).to.have.length(defaultItems.length + 1);
    expect($('#table thead').html()).to.be(
      '<tr><th></th>' +
      '<th style="text-align: center" colspan="1">Zabbix1</th>' +
      '<th style="text-align: center" colspan="1">Zabbix2</th></tr>' +
      '<tr><th></th><th>Host1</th><th>Host2</th></tr>');
    expect($('#table tbody').html()).to.be(
      '<tr><th>cpu usage</th><td>54.28 %</td><td>23.21 %</td></tr>');
  });
});
