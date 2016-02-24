(function(hatohol) {
  var self = hatohol.registerPlugin("aa25a332-d1f7-11e4-80b4-d43d7e3146fb",
                                    "Ceilometer (HAPI2)", 65);
  self.getHostName = function(server, hostId) {
    if (hostId == "N/A")
      return "N/A";
    return null;
  };
}(hatohol));
