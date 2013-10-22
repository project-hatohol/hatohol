describe('HatoholSessionManager', function() {
  beforeEach(function(done) {
    HatoholSessionManager.deleteCookie();
    done();
  });

  it('get', function() {
    expect(HatoholSessionManager.get()).to.be(null);
  });

  it('set', function() {
    var TEST_SID = "93328d5e-5b82-4c98-9466-f4d4171af6b5";
    expect(HatoholSessionManager.set(TEST_SID));
    expect(HatoholSessionManager.get()).to.be(TEST_SID);
  });
});

