describe('HatoholNavi', function() {
  function adminUser() {
    return {
      "userId": 1,
      "name": "admin",
      "flags": hatohol.ALL_PRIVILEGES
    };
  }

  function guestUser() {
    return {
      "userId": 23,
      "name": "guest",
      "flags": 0
    };
  }

  beforeEach(function(done) {
    var nav = $("<ul/>").addClass("nav");
    $("body").append(nav);
    done();
  });

  afterEach(function(done) {
    $(".nav").remove();
    done();
  });
            
  it('show users against admin', function() {
    var nav = new HatoholNavi(adminUser());
    var links = $("a[href = 'ajax_users']");
    expect(links.length).to.be(1);
    expect(links[0].text).to.be(gettext("Users"));
  });
            
  it('do not show users against guest', function() {
    var nav = new HatoholNavi(guestUser());
    var links = $("a[href = 'ajax_users']");
    expect(links.length).to.be(0);
  });
});
