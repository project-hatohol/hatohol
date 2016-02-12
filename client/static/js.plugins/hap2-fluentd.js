(function(hatohol) {
  var self = hatohol.registerPlugin("d91ba1cb-a64a-4072-b2b8-2f91bcae1818",
                                    "Fluentd (HAPI2)", 70);
  self.getHostName = function(server, hostId) {
    return hostId;
  }
}(hatohol));
