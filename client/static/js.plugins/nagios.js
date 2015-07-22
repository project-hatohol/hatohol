(function(hatohol) {
  var type = hatohol.MONITORING_SYSTEM_NAGIOS;
  var self = hatohol.addNamespace("hatohol.hap_" + type);

  self.type = type;
  self.label = "Nagios";

  self.getTopURL = function(server) {
    if (!server)
      return undefined;
    return server["baseURL"];
  };
}(hatohol));
