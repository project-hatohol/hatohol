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
      "prevValue": "24.594534",
      "itemGroupName": "group1",
      "unit": "%",
      "valueType": hatohol.ITEM_INFO_VALUE_TYPE_FLOAT,
    },
  ];
  var defaultServers = {
    "1": {
      "name": "Zabbix",
      "type": 0,
      "ipAddress": "192.168.1.100",
      "hosts": {
        "10101": {
          "name": "Host1",
        },
      },
    },
  };

  function itemsJson(items, servers) {
    return JSON.stringify({
      apiVersion: 3,
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

  function respondUserConfig(configJson) {
    var request = this.requests[0];
    if (!configJson)
      configJson = '{}';
    request.respond(200, { "Content-Type": "application/json" },
                    configJson);
  }

  function respondItems(itemsJson) {
    var request = this.requests[1];
    request.respond(200, { "Content-Type": "application/json" },
                    itemsJson);
  }

  function respond(itemsJson, configJson) {
    respondUserConfig(configJson);
    respondItems(itemsJson);
  }
  
  beforeEach(function(done) {
    var contentId = "main";
    var setupFixture = function() {
      $("#" + TEST_FIXTURE_ID).append($("<div>", { id: contentId }));
      $("#" + contentId).html(viewHTML);
      fakeAjax();
      done();
    };

    $('body').append($('<div>', { id: TEST_FIXTURE_ID }));

    if (viewHTML) {
      setupFixture();
    } else {
      var iframe = $("<iframe>", {
        id: "fixtureFrame",
        src: "../../ajax_latest?start=false",
        load: function() {
          viewHTML = $("#" + contentId, this.contentDocument).html();
          setupFixture();
        }
      });
      $("#" + TEST_FIXTURE_ID).append(iframe);
    }
  });

  afterEach(function() {
    restoreAjax();
    $("#" + TEST_FIXTURE_ID).remove();
  });

  it('new with empty data', function() {
    var view = new LatestView($('#' + TEST_FIXTURE_ID).get(0));
    respond(itemsJson());
    var heads = $('div#' + TEST_FIXTURE_ID + ' h2');
    expect(heads).to.have.length(1);
    expect($('#table')).to.have.length(1);
  });

  it('An item row', function() {
    var view = new LatestView($('#' + TEST_FIXTURE_ID).get(0));
    respond(itemsJson(defaultItems, defaultServers));
    expect($('#table')).to.have.length(1);
    expect($('tr')).to.have.length(defaultItems.length + 1);
    expect($('tr :eq(1)').html()).to.be(
      '<td>Zabbix</td>' +
      '<td>Host1</td>' +
      '<td>group1</td>' +
      '<td><a href="http://192.168.1.100/zabbix/history.php?action=showgraph&amp;itemid=1">cpu usage</a></td>' +
      '<td data-sort-value="1415232279">2014/11/06 09:04:39</td>' +
      '<td>54.28 %</td>' +
      '<td>24.59 %</td>'
    );
  });

  it('default page size', function() {
    var view = new LatestView($('#' + TEST_FIXTURE_ID).get(0));
    respond(itemsJson());
    expect($('#num-records-per-page').val()).to.be("50");
  });
});
