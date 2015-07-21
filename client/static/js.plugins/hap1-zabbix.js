(function(hatohol) {
  var type = hatohol.MONITORING_SYSTEM_HAPI_ZABBIX;
  var self = hatohol.addNamespace("hatohol.hap_" + type);

  self.type = type;
  self.label = "Zabbix (HAPI)";

  self.getTopURL = function(server) {
    var ipAddress, port, url;

    if (!server)
      return undefined;

    ipAddress = server["ipAddress"];
    port = server["port"];
    if (hatohol.isIPv4(ipAddress))
      url = "http://" + ipAddress;
    else // maybe IPv6
      url = "http://[" + ipAddress + "]";
    if (!isNaN(port) && port != "80")
      url += ":" + port;
    url += "/zabbix/";

    return url;
  };

  self.getItemGraphURL = function(server, itemId) {
    var url = self.getTopURL(server);
    if (!url)
      return undefined;

    url += "history.php?action=showgraph&itemid=" + itemId;
    return url;
  };

  self.getMapsURL = function(server) {
    var url = self.getTopURL(server);
    if (!url)
      return undefined;

    url += "maps.php";
    return url;
  };
}(hatohol));
