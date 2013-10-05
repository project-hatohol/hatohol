function hatohol_base_js_load(mainFunction) {

  var exjs = [
    "jquery.js",
    "jquery-ui.min.js"
  ];

  var js = [
    "bootstrap.js",
    "library.js",
    "hatohol_def.js",
    "hatohol_dialog.js",
    "hatohol_login_dialog.js"
  ];

  var excss = [
    "themes/ui-lightness/jquery-ui.css", 
  ];

  var css = [
    "bootstrap.css",
    "zabbix.css"
  ];

  var stages = [exjs, js, excss, css];
  var currStageIndex = 0;
  var currIndex = 0;
  var complete = false;

  // start
  loadChain();

  function loadChain() {
    if (complete) {
      if (mainFunction)
        mainFunction();
      return;
    }

    loadCurr();
    currIndex++;
    if (currIndex == stages[currStageIndex].length) {
      currIndex = 0;
      currStageIndex++;
    } 
    if (currStageIndex == stages.length)
      complete = true;
  }

  function loadCurr() {
    switch (stages[currStageIndex]) {
    case exjs:
      addScript("../static/js.external/" + exjs[currIndex]);
      break;
    case js:
      addScript("../static/js/" + js[currIndex]);
      break;
    case excss:
      addLink("../static/css.external/" + excss[currIndex]);
      break;
    case css:
      addLink("../static/css/" + css[currIndex]);
    }
  }

  function addScript(path) {
    var elem = document.createElement("script");
    elem.type = "text/javascript";
    elem.src = path;
    elem.onload = loadChain;
    document.body.appendChild(elem);
  }

  function addLink(path) {
    var elem = document.createElement("link");
    elem.href = path;
    elem.rel = "stylesheet";
    elem.onload = loadChain;
    document.body.appendChild(elem);
  }
}

function gettext(msg)
{
    return msg;
}
