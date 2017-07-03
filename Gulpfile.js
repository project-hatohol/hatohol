var gulp = require("gulp");
var mochaPhantomJS = require('gulp-mocha-phantomjs');

gulp.task('browsertest', function () {
  var phantomJSConfig = {
    reporter: 'spec',
    phantomjs: {
      viewportSize: {
        width: 1024,
        height: 768
      },
      useColors:true,
      suppressStdout: false,
      suppressStderr: false,
      verbose: true
    }
  };
  var stream = mochaPhantomJS(phantomJSConfig);
  stream.write({path: 'http://localhost:8000/test/index.html'});
  stream.end();
  return stream;
});
