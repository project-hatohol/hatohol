(function(hatohol) {
  var self = hatohol.addNamespace("hatohol.hap_1");
  // 1 == hatohol.MONITORING_SYSTEM_NAGIOS

  self.type = hatohol.MONITRING_SYSETEM_NAGIOS;
  self.label = "Nagios";

  self.getTopURL = function(server) {
    var url;

    if (!server)
      return undefined;

    if (server["baseURL"]) {
      url = server["baseURL"];
    } else {
      url = undefined; // issue-839

    }
    return url ? hatohol.escapeHTML(url) : url;
  };
}(hatohol));
