(function(hatohol) {
  var uuid = "aa25a332-d1f7-11e4-80b4-d43d7e3146fb";
  var self = hatohol.addNamespace("hatohol.hap_" + uuid);

  self.type = uuid;
  self.label = "Ceilometer (HAPI2)";
  self.getHostName = function(server, hostId) {
    if (hostId == "N/A")
        return "N/A"
    return null
  }
}(hatohol));
