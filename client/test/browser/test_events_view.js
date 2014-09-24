describe('EventsView', function() {
  var TEST_FIXTURE_ID = 'eventsViewFixture';
  var viewHTML;

  // TODO: we should use actual server response to follow changes of the json
  //       format automatically
  function eventsJson(events, servers) {
    return JSON.stringify({
      apiVersion: 3,
      errorCode: hatohol.HTERR_OK,
      events: events ? events : [],
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

  function respondEvents(eventsJson) {
    var request = this.requests[1];
    request.respond(200, { "Content-Type": "application/json" },
                    eventsJson);
  }

  function respond(eventsJson, configJson) {
    respondUserConfig(configJson);
    respondEvents(eventsJson);
  }
  
  beforeEach(function(done) {
    var contentId = "main";
    var setupFixture = function() {
      $("#" + TEST_FIXTURE_ID).append($("<div>", { id: contentId }))
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
        src: "../../ajax_events?start=false",
        load: function() {
          viewHTML = $("#" + contentId, this.contentDocument).html();
          setupFixture();
        }
      })
      $("#" + TEST_FIXTURE_ID).append(iframe);
    }
  });

  afterEach(function() {
    restoreAjax();
    $("#" + TEST_FIXTURE_ID).remove();
  });

  it('new with empty data', function() {
    var view = new EventsView($('#' + TEST_FIXTURE_ID).get(0));
    respond(eventsJson());
    var heads = $('div#' + TEST_FIXTURE_ID + ' h2');
    expect(heads).to.have.length(1);
    //TODO: It requires valid gettext()
    //expect(heads.first().text()).to.be(gettext("event"));
    expect($('#table')).to.have.length(1);
    expect($('#num-events-per-page').val()).to.be("50");
  });
});
