(function(hatohol) {
  var self = hatohol.registerPlugin(hatohol.MONITORING_SYSTEM_NAGIOS,
                                    "Nagios", 95);

  self.getTopURL = function(server) {
    if (!server)
      return undefined;
    return server["baseURL"];
  };
}(hatohol));
