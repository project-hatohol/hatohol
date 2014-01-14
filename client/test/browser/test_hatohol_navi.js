describe('HatoholNavi', function() {
  var adminUser = {
    "userId": 1,
    "name": "admin",
    "flags": hatohol.ALL_PRIVILEGES
  };

  var guestUser = {
    "userId": 23,
    "name": "guest",
    "flags": 0
  };

  beforeEach(function() {
    var nav = $("<ul/>").addClass("nav");
    $("body").append(nav);
  });

  afterEach(function() {
    $(".nav").remove();
  });

  it('show users against admin', function() {
    var nav = new HatoholNavi(adminUser);
    var links = $("a[href = 'ajax_users']");
    expect(links.length).to.be(1);
    expect(links[0].text).to.be(gettext("Users"));
  });

  it('do not show users against guest', function() {
    var nav = new HatoholNavi(guestUser);
    var links = $("a[href = 'ajax_users']");
    expect(links.length).to.be(0);
  });

  it('with no current page', function() {
    var nav = new HatoholNavi(guestUser);
    var expected = '';
    expected += '<li><a href="ajax_dashboard">' +
      gettext('Dashboard') + '</a></li>';
    expected += '<li><a href="ajax_overview_triggers">' +
      gettext('Overview : Triggers') + '</a></li>';
    expected += '<li><a href="ajax_overview_items">' +
      gettext('Overview : Items') + '</a></li>';
    expected += '<li><a href="ajax_latest">' +
      gettext("Latest data") + '</a></li>';
    expected += '<li><a href="ajax_triggers">' +
      gettext('Triggers') + '</a></li>';
    expected += '<li><a href="ajax_events">' +
      gettext('Events') + '</a></li>';
    expected += '<li><a href="ajax_servers">' +
      gettext('Servers') + '</a></li>';
    expected += '<li><a href="ajax_actions">' +
      gettext('Actions') + '</a></li>';
    expect($("ul.nav")[0].innerHTML).to.be(expected);
  });

  it('with a current page argument', function() {
    var nav = new HatoholNavi(guestUser, "ajax_latest");
    var expected = '';
    expected += '<li><a href="ajax_dashboard">' +
      gettext('Dashboard') + '</a></li>';
    expected += '<li><a href="ajax_overview_triggers">' +
      gettext('Overview : Triggers') + '</a></li>';
    expected += '<li><a href="ajax_overview_items">' +
      gettext('Overview : Items') + '</a></li>';
    expected += '<li class="active"><a>' +
      gettext('Latest data') + '</a></li>';
    expected += '<li><a href="ajax_triggers">' +
      gettext('Triggers') + '</a></li>';
    expected += '<li><a href="ajax_events">' +
      gettext('Events') + '</a></li>';
    expected += '<li><a href="ajax_servers">' +
      gettext('Servers') + '</a></li>';
    expected += '<li><a href="ajax_actions">' +
      gettext('Actions') + '</a></li>';
    expect($("ul.nav")[0].innerHTML).to.be(expected);
  });
});
