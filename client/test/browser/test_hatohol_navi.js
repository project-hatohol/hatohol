describe('HatoholNavi', function() {
  var adminUser = new HatoholUserProfile({
    "userId": 1,
    "name": "admin",
    "flags": hatohol.ALL_PRIVILEGES
  });

  var guestUser = new HatoholUserProfile({
    "userId": 23,
    "name": "guest",
    "flags": 0
  });

  function domesticAnchorList(uri, label) {
    return '<li>' + anchorTagForDomesticLink(uri, label) + '</li>';
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
    expect(links).to.have.length(1);
    expect(links[0].text).to.be(gettext("Users"));
  });

  it('do not show users against guest', function() {
    var nav = new HatoholNavi(guestUser);
    var links = $("a[href = 'ajax_users']");
    expect(links).to.be.empty();
  });

  it('with no current page', function() {
    var userProfile = new HatoholUserProfile(guestUser);
    var nav = new HatoholNavi(userProfile);
    var expected = '';
    expected += domesticAnchorList("ajax_dashboard", gettext('Dashboard'));
    expected += domesticAnchorList("ajax_overview_triggers",
                                   gettext('Overview : Triggers'));
    expected += domesticAnchorList("ajax_overview_items",
                                   gettext('Overview : Items'));
    expected += domesticAnchorList("ajax_latest", gettext("Latest data"));
    expected += domesticAnchorList("ajax_triggers", gettext('Triggers'));
    expected += domesticAnchorList("ajax_events", gettext('Events'));
    expected += '<li class="dropdown">';
    expected += '<a href="#" class="dropdown-toggle" data-toggle="dropdown">' +
      'Settings<span class="caret"></span></a>';
    expected += '<ul class="dropdown-menu">';
    expected += domesticAnchorList("ajax_servers",
                                   gettext('Monitoring Servers'));
    expected += domesticAnchorList("ajax_actions", gettext('Actions'));
    expected += domesticAnchorList("ajax_graphs", gettext('Graphs'));
    expected += domesticAnchorList("ajax_log_search_systems",
                                   gettext('Log search systems'));
    expected += '</ul></li>';
    expected += '<li class="dropdown">';
    expected += '<a href="#" class="dropdown-toggle" data-toggle="dropdown">' +
      'Help<span class="caret"></span></a>';
    expected += '<ul class="dropdown-menu">';
    expected += '<li><a href="http://www.hatohol.org/docs" target="_blank">' +
                gettext('Online Documents') + '</a></li>';
    expected += '<li><a href="#version" onclick="return false">' +
                gettext('Hatohol version: ') + HATOHOL_VERSION + '</a></li>';
    expected += '</ul></li>';

    expect($("ul.nav")[0].innerHTML).to.be(expected);
  });

  it('with a current page argument', function() {
    var nav = new HatoholNavi(guestUser, "ajax_latest");
    var expected = '';

    expected += domesticAnchorList("ajax_dashboard", gettext('Dashboard'));
    expected += domesticAnchorList("ajax_overview_triggers",
                                   gettext('Overview : Triggers'));
    expected += domesticAnchorList("ajax_overview_items",
                                   gettext('Overview : Items'));
    expected += '<li class="active"><a>' +
      gettext('Latest data') + '</a></li>';
    expected += domesticAnchorList("ajax_triggers", gettext('Triggers'));
    expected += domesticAnchorList("ajax_events", gettext('Events'));
    expected += '<li class="dropdown">';
    expected += '<a href="#" class="dropdown-toggle" data-toggle="dropdown">' +
      'Settings<span class="caret"></span></a>';
    expected += '<ul class="dropdown-menu">';
    expected += domesticAnchorList("ajax_servers",
                                   gettext('Monitoring Servers'));
    expected += domesticAnchorList("ajax_actions", gettext('Actions'));
    expected += domesticAnchorList("ajax_graphs", gettext('Graphs'));
    expected += domesticAnchorList("ajax_log_search_systems",
                                   gettext('Log search systems'));
    expected += '</ul></li>';
    expected += '<li class="dropdown">';
    expected += '<a href="#" class="dropdown-toggle" data-toggle="dropdown">' +
      'Help<span class="caret"></span></a>';
    expected += '<ul class="dropdown-menu">';
    expected += '<li><a href="http://www.hatohol.org/docs" target="_blank">' +
                gettext('Online Documents') + '</a></li>';
    expected += '<li><a href="#version" onclick="return false">' +
                gettext('Hatohol version: ') + HATOHOL_VERSION + '</a></li>';
    expected += '</ul></li>';

    expect($("ul.nav")[0].innerHTML).to.be(expected);
  });
});
