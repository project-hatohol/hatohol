(function(){
  "use strict";
  var permissionLevel;
  var importantEventCount = 0;
  var noMonitoringTime = 0;
  var timeLimit = 300;
  var pushInterval = 35;

  function severityMonitor() {
    if (
      noMonitoringTime >= timeLimit &&
      (noMonitoringTime - timeLimit) % pushInterval === 0
    ) {
      pushWarning();
    }
    noMonitoringTime ++;
  }

  function pushWarning() {
    importantEventCount = parseInt($("#numOfUnAssignedEvents").text());
    var fmts = ngettext(
      'There is %s unmet important event.',
      'There are %s unmet important events.',
      importantEventCount
    );
    var alertBodyText = interpolate(fmts, [importantEventCount]);
    if (importantEventCount > 0) {
      notify.createNotification(
        gettext('Failure has occurred.'),
        {
          body: alertBodyText,
          icon: 'static/images/icon_push_hatohol.png'
        }
      );
    }
  }

  $(document).on('mousemove mousedown click keydown focus', importantEventCountReset);

  function importantEventCountReset() {
    noMonitoringTime = 0;
    importantEventCount = 0;
  }
  $(function() {
    if (notify.isSupported) {
      if (notify.permissionLevel() !== notify.PERMISSION_GRANTED) {
        notify.requestPermission (function() {
          permissionLevel = notify.permissionLevel();
        });
      } else {
        permissionLevel = notify.permissionLevel();
      }

      if (permissionLevel === "granted") {
        setInterval(severityMonitor, 1000);
      }
    }
  });
})();
