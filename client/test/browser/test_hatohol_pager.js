describe('HatoholPager', function() {
  var fixtureId = 'fixture';

  function getTestParams(params)
  {
     var baseParams = {
       numRecordsPerPage: 10,
       numTotalRecords: 200,
       currentPage: 0,
    };
    return $.extend(baseParams, params)
  }

  beforeEach(function() {
    $('body').append('<ul id="' + fixtureId + '" class="pagination"></ul>');
  });
  afterEach(function() {
    $('#' + fixtureId).remove();
  });

  it('do not show', function() {
    var pager = new HatoholPager({numTotalRecords: 50});
    var i, list = $('#' + fixtureId + ' li');
    expect(list.length).to.be(0);
  });

  it('create with 100 records', function() {
    var pager = new HatoholPager({numTotalRecords: 100});
    var i, list = $('#' + fixtureId + ' li');
    expect(list.length).to.be(3);
    for (i = 0; i < 2; i++)
      expect($(list[i]).text()).to.be("" + (i + 1));
    expect($(list[2]).text()).to.be($('<div/>').html("&raquo;").text());
  });

  it('create with 101 records', function() {
    var pager = new HatoholPager({numTotalRecords: 101});
    var i, list = $('#' + fixtureId + ' li');
    expect(list.length).to.be(4);
    for (i = 0; i < 3; i++)
      expect($(list[i]).text()).to.be("" + (i + 1));
    expect($(list[3]).text()).to.be($('<div/>').html("&raquo;").text());
  });

  it('0 records per page', function() {
    var pager = new HatoholPager(getTestParams({
      numRecordsPerPage: 0,
      numTotalRecords: 10
    }));
    expect(pager.getTotalNumberOfPages()).to.eql(1);
  });

  it('0 records per page agaist 0 total records', function() {
    var pager = new HatoholPager(getTestParams({
      numRecordsPerPage: 0,
      numTotalRecords: 0
    }));
    expect(pager.getTotalNumberOfPages()).to.eql(0);
  });

  it('negative value for number of records per page', function() {
    var pager = new HatoholPager(getTestParams({
      numRecordsPerPage: -1,
      numTotalRecords: 10
    }));
    expect(pager.getTotalNumberOfPages()).to.eql(1);
  });

  it('6 / 20 pages', function() {
    var pager = new HatoholPager(getTestParams({ currentPage: 5 }));
    expect(pager.getPagesRange()).to.eql({
      firstPage: 0,
      lastPage: 9,
    });
  });

  it('7 / 20 pages', function() {
    var pager = new HatoholPager(getTestParams({ currentPage: 6 }));
    expect(pager.getPagesRange()).to.eql({
      firstPage: 1,
      lastPage: 10,
    });
  });

  it('15 / 20 pages', function() {
    var params = getTestParams({ currentPage: 14 });
    var pager = new HatoholPager(params);
    expect(pager.getPagesRange()).to.eql({
      firstPage: 9,
      lastPage: 18,
    });
  });

  it('16 / 20 pages', function() {
    var params = getTestParams({ currentPage: 15 });
    var pager = new HatoholPager(params);
    expect(pager.getPagesRange()).to.eql({
      firstPage: 10,
      lastPage: 19,
    });
  });

  it('17 / 20 pages', function() {
    var params = getTestParams({ currentPage: 16 });
    var pager = new HatoholPager(params);
    expect(pager.getPagesRange()).to.eql({
      firstPage: 10,
      lastPage: 19,
    });
  });

  it('6 / 6 pages', function() {
    var params = getTestParams({
      numTotalRecords: 60,
      currentPage: 5,
    });
    var pager = new HatoholPager(params);
    expect(pager.getPagesRange()).to.eql({
      firstPage: 0,
      lastPage: 5,
    });
  });
});
