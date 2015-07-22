(function(hatohol) {
  var self = hatohol.registerPlugin(hatohol.MONITORING_SYSTEM_NAGIOS,
                                    "Nagios");

  self.getTopURL = function(server) {
    if (!server)
      return undefined;
    return server["baseURL"];
  };
}(hatohol));
