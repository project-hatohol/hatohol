function login() {
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
  casper.waitForSelector("form#loginForm input[type=submit][value='ログイン']",
    function success() {
      this.click("form#loginForm input[type=submit][value='ログイン']");
    },
    function fail() {
      test.assertExists("form#loginForm input[type=submit][value='ログイン']");
    });
};
exports.login = login;

function logout() {
  casper.then(function() {
    this.click('#userProfileButton');
    this.waitForSelector('#logoutMenuItem', function() {
      this.click('#logoutMenuItem');
    });
  });
};
exports.logout = logout;
