describe('HatoholMonitoringView', function() {
  it('set filter candidates', function() {
    var candidates = [
      { label: 'apple',  value: 1 },
      { label: 'orange', value: 2 },
      { label: 'lemon',  value: 3 }
    ];
    var target = $('<select>');
    var setCandidates = HatoholMonitoringView.prototype.setFilterCandidates;
    var expected = '<option>---------</option>';
    var i;
    for (i = 0; i < candidates.length; i++) {
      expected += '<option value="' + candidates[i].value + '">' +
                  candidates[i].label + "</option>";
    }
    setCandidates(target, candidates);
    expect(target.html()).to.be(expected);
  });

  it('set filter candidates', function() {
    var candidates = [ 'apple', 'orange', 'lemon' ];
    var target = $('<select>');
    var setCandidates = HatoholMonitoringView.prototype.setFilterCandidates;
    var expected = '<option>---------</option>';
    var i;
    for (i = 0; i < candidates.length; i++)
      expected += '<option>' + candidates[i] + "</option>";
    setCandidates(target, candidates);
    expect(target.html()).to.be(expected);
  });

  it('set empty filter', function() {
    var target = $('<select>');
    var setCandidates = HatoholMonitoringView.prototype.setFilterCandidates;
    setCandidates(target);
    var expected = '<option>---------</option>';
    expect(target.html()).to.be(expected);
  });
});
