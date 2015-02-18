function login(test) {
  casper.waitForSelector("input#inputUserName",
    function success() {
      this.sendKeys("input#inputUserName", "admin");
    });
  casper.waitForSelector("input#inputPassword",
    function success() {
      this.sendKeys("input#inputPassword", "hatohol");
    },
    function fail() {
      test.assertExists("input#inputPassword");
    });
  casper.then(function() {
    this.click('input#loginFormSubmit');
  });
};
exports.login = login;

function logout(test) {
  casper.then(function() {
    this.click('#userProfileButton');
    this.waitForSelector('#logoutMenuItem', function() {
      this.click('#logoutMenuItem');
    });
  });
};
exports.logout = logout;
