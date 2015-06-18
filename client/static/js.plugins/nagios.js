(function(hatohol) {
  var type = hatohol.MONITORING_SYSTEM_NAGIOS;
  var self = hatohol.addNamespace("hatohol.hap_" + type);

  self.type = type;
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
