describe('HatoholPager', function() {
  var fixtureId = 'fixture';
  beforeEach(function() {
    $('body').append('<ul id="' + fixtureId + '" class="pagination"></ul>');
  });
  afterEach(function() {
    $('#' + fixtureId).remove();
  });

  it('create with 100 records', function() {
    var pager = new HatoholPager({numTotalRecords: 100});
    var list = $('#' + fixtureId + ' li');
    expect(list.length).to.be(4);
    expect($(list[0]).text()).to.be($('<div/>').html("&laquo;").text());
    for (i = 0; i < 2; i++) {
      expect($(list[i + 1]).text()).to.be("" + (i + 1));
    }
    expect($(list[3]).text()).to.be($('<div/>').html("&raquo;").text());
  });

  it('create with 101 records', function() {
    var pager = new HatoholPager({numTotalRecords: 101});
    var list = $('#' + fixtureId + ' li');
    expect(list.length).to.be(5);
    expect($(list[0]).text()).to.be($('<div/>').html("&laquo;").text());
    for (i = 0; i < 3; i++) {
      expect($(list[i + 1]).text()).to.be("" + (i + 1));
    }
    expect($(list[4]).text()).to.be($('<div/>').html("&raquo;").text());
  });
});