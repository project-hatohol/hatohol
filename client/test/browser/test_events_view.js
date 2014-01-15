describe('EventsView', function() {
  var TEST_FIXTURE_ID = 'eventsViewFixture';

  beforeEach(function() {
    var fixture = $('<div/>', {id: TEST_FIXTURE_ID});
    $("body").append(fixture);
  });

  afterEach(function() {
    $("#" + TEST_FIXTURE_ID).remove();
  });

  it('new with empty data', function() {
    var xhr = sinon.useFakeXMLHttpRequest();
    var requests = [];
    xhr.onCreate = function(xhr) {
      requests.push(xhr);
    };
    var view = new EventsView($('#' + TEST_FIXTURE_ID).get(0));
    // userconfig
    requests[0].respond(200, { "Content-Type": "application/json" },
                        '{}');
    // events
    requests[1].respond(200, { "Content-Type": "application/json" },
                        '{ "events":[], "servers":[] }');
    var heads = $('div#' + TEST_FIXTURE_ID + ' h2');
    expect(heads).to.have.length(1);
    expect(heads.first().text()).to.be(gettext("Events"));
    expect($('#table')).to.have.length(1);
    expect($('#num-events-per-page').val()).to.be("50");
    xhr.restore();
  });
});
