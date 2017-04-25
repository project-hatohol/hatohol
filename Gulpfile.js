var gulp = require("gulp");
var jshint = require("gulp-jshint");
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

gulp.task("lint", function() {
  gulp.src(["./client/static/js/*.js", "./client/static/js.plugins/*.js",
            "!./client/static/js/hatohol_def.js"])
    .pipe(jshint())
    .pipe(jshint.reporter("default"))
    .pipe(jshint.reporter("fail"));
});

gulp.task('copy', function() {
  // Copy external JavaScript files
  gulp.src([
    'node_modules/bootstrap/dist/js/bootstrap.min.js',
    'node_modules/bootstrap-select/dist/js/bootstrap-select.js',
    'node_modules/jquery/dist/jquery.min.js',
    'node_modules/jquery-flot/jquery.flot.js',
    'node_modules/jquery-flot/jquery.flot.selection.js',
    'node_modules/jquery-flot/jquery.flot.time.js',
    'node_modules/spectrum-colorpicker/spectrum.js',
  ])
  .pipe(gulp.dest('client/static/js.external'));
  // Copy external CSS files
  gulp.src([
    'node_modules/bootstrap/dist/css/bootstrap.min.css',
    'node_modules/bootstrap-select/dist/css/bootstrap-select.css',
  ])
  .pipe(gulp.dest('client/static/css.external'));
  // Copy external fonts files
  gulp.src(['node_modules/bootstrap/dist/fonts/*'])
  .pipe(gulp.dest('client/static/fonts'));
});

gulp.task('default', ['lint']);
gulp.task('test', ['copy', 'browsertest'], function(){
  verbose: true;
});
