(function(hatohol) {
  var self = hatohol.registerPlugin(hatohol.MONITORING_SYSTEM_NAGIOS,
                                    "Nagios");

  self.getTopURL = function(server) {
    var url;

    if (!server)
      return undefined;

    if (server["baseURL"]) {
      url = server["baseURL"];
    } else {
      url = undefined; // issue-839
    }
    return url;
  };
}(hatohol));
