(function(global, namespace) {
  var self = hatohol.addNamespace(namespace);

  self.type = hatohol.MONITRING_SYSETEM_ZABBIX;
  self.label = "Zabbix";

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

    return url ? hatohol.escapeHTML(url) : url;
  };

  self.getItemGraphURL = function(server, itemId) {
    var location = self.getTopURL(server);
    if (!location)
      return undefined;

    location += "history.php?action=showgraph&amp;itemid=" + itemId;
    return location;
  };

  self.getMapsURL = function(server) {
    var location = self.getTopURL(server);
    if (!location)
      return undefined;

    location += "maps.php";
    return location;
  };
}(this, "hatohol.hap_0")); // 0 == hatohol.MONITORING_SYSTEM_ZABBIX
