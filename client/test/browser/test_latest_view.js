describe('LatestView', function() {
  var TEST_FIXTURE_ID = 'latestViewFixture';
  var viewHTML;
  var defaultItems = [
    {
      "id": 1,
      "serverId": 1,
      "hostId": "10101",
      "brief": "cpu usage",
      "lastValueTime": 1415232279,
      "lastValue": "54.282349",
      "itemGroupName": "group1",
      "unit": "%",
      "valueType": hatohol.ITEM_INFO_VALUE_TYPE_FLOAT,
    },
    {
      "id": 2,
      "serverId": 1,
      "hostId": "10101",
      "brief": "host name",
      "lastValueTime": 1415232279,
      "lastValue": "host1",
      "itemGroupName": "group1",
      "unit": "",
      "valueType": hatohol.ITEM_INFO_VALUE_TYPE_STRING,
    },
  ];
  var defaultServers = {
    "1": {
      "nickname": "Zabbix",
      "type": 0,
      "ipAddress": "192.168.1.100",
      "hosts": {
        "10101": {
          "name": "Host1",
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

  function itemsJson(items, servers, applications) {
    return JSON.stringify({
      apiVersion: 3,
      errorCode: hatohol.HTERR_OK,
      items: items ? items : [],
      servers: servers ? servers : {},
      applications: applications ? applications : []
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

  function respond(configJson, itemsJson) {
    var header = { "Content-Type": "application/json" };
    this.requests[0].respond(200, header, configJson);
    this.requests[1].respond(200, header, itemsJson);
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
        src: "../../ajax_latest?start=false",
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
    var view = new LatestView(getOperator());
    respond('{}', itemsJson());
    var heads = $('div#' + TEST_FIXTURE_ID + ' h2');
    expect(heads).to.have.length(1);
    expect($('#table')).to.have.length(1);
  });

  it('Float item', function() {
    var view = new LatestView(getOperator());
    var zabbixURL = "http://192.168.1.100/zabbix/history.php?action=showgraph&amp;itemid=1";
    var historyURL= "ajax_history?serverId=1&amp;hostId=10101&amp;itemId=1";
    var expected =
      '<td>Zabbix</td>' +
      '<td>Host1</td>' +
      '<td>group1</td>' +
      '<td><a href="' + zabbixURL + '" target="_blank">cpu usage</a></td>' +
      '<td data-sort-value="1415232279">' +
      formatDate(1415232279) +
      '</td>' +
      '<td>54.28 %</td>' +
      '<td>24.59 %</td>'+
      '<td><a href="' + historyURL + '">Graph</a></td>';
    respond('{}', itemsJson(defaultItems, defaultServers));
    expect($('#table')).to.have.length(1);
    expect($('tr')).to.have.length(defaultItems.length + 1);
    expect($('tr :eq(1)').html()).to.be(expected);
  });

  it('String item', function() {
    var view = new LatestView(getOperator());
    var zabbixURL = "http://192.168.1.100/zabbix/history.php?action=showgraph&amp;itemid=2";
    var expected =
      '<td>Zabbix</td>' +
      '<td>Host1</td>' +
      '<td>group1</td>' +
      '<td><a href="' + zabbixURL + '" target="_blank">host name</a></td>' +
      '<td data-sort-value="1415232279">' +
      formatDate(1415232279) +
      '</td>' +
      '<td>host1</td>' +
      '<td>host1</td>'+
      '<td></td>';
    respond('{}', itemsJson(defaultItems, defaultServers));
    expect($('#table')).to.have.length(1);
    expect($('tr')).to.have.length(defaultItems.length + 1);
    expect($('tr :eq(2)').html()).to.be(expected);
  });

  it('default page size', function() {
    var view = new LatestView(getOperator());
    respond('{}', itemsJson());
    expect($('#num-records-per-page').val()).to.be("50");
  });
});
