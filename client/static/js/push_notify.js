$(function() {
  "use strict";
  var permissionLevel;
  var importantEventCount = 0;
  var noMonitoringTime = 0;
  var timeLimit = 10;
  var pushInterval = 20;
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

      function severityMonitor() {
        noMonitoringTime ++;
        if (
          noMonitoringTime >= timeLimit &&
          (noMonitoringTime - timeLimit) % pushInterval === 0
        ) {
          pushWarning();
        }
      }

      function pushWarning() {
        importantEventCount ++;
        if (importantEventCount > 0) {
          notify.createNotification(
            gettext('Failure has occurred.'),
            {
              body: gettext('There are ' + importantEventCount + ' unmet important events .'),
              icon: '/static/images/icon_push_hatohol.png'
            }
          );
        }
      }

      $(document).on('mousemove mousedown click keydown focus', importantEventCountReset);

      function importantEventCountReset() {
        noMonitoringTime = 0;
        importantEventCount = 0;
      }
    }
  }
});