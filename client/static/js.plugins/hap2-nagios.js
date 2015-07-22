(function(hatohol) {
  var uuid = "902d955c-d1f7-11e4-80f9-d43d7e3146fb";
  var self = hatohol.addNamespace("hatohol.hap_" + uuid);

  self.type = uuid;
  self.label = "Nagios (HAPI2)";

  self.getTopURL = function(server) {
    if (!server)
      return undefined;
    return server["baseURL"];
  };
}(hatohol));
