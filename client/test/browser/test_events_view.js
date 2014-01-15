describe('EventsView', function() {
  var TEST_FIXTURE_ID = 'eventsViewFixture';

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
    if (!configJson)
      configJson = '{}';
    requests[0].respond(200, { "Content-Type": "application/json" },
                        configJson);
  }

  function respondEvents(eventsJson) {
    requests[1].respond(200, { "Content-Type": "application/json" },
                        eventsJson);
  }

  function respond(eventsJson, configJson) {
    respondUserConfig(configJson);
    respondEvents(eventsJson);
  }
  
  beforeEach(function() {
    var fixture = $('<div/>', {id: TEST_FIXTURE_ID});
    $("body").append(fixture);
    fakeAjax();
  });

  afterEach(function() {
    restoreAjax();
    $("#" + TEST_FIXTURE_ID).remove();
  });

  it('new with empty data', function() {
    var view = new EventsView($('#' + TEST_FIXTURE_ID).get(0));
    respond('{ "events":[], "servers":[] }');
    var heads = $('div#' + TEST_FIXTURE_ID + ' h2');
    expect(heads).to.have.length(1);
    expect(heads.first().text()).to.be(gettext("Events"));
    expect($('#table')).to.have.length(1);
    expect($('#num-events-per-page').val()).to.be("50");
  });
});
