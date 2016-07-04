(function(hatohol) {
  var self = hatohol.registerPlugin("8e632c14-d1f7-11e4-8350-d43d7e3146fb",
                                    "Zabbix (HAPI2)", 85);

  self.getTopURL = function(server) {
    var url, suffix = "api_jsonrpc.php";
    var suffixPos;

    if (!server)
      return undefined;

    url = server["baseURL"];
    suffixPos = url.indexOf(suffix);
    if (suffixPos == url.length - suffix.length)
      return url.substr(0, suffixPos);
    return undefined;
  };

  self.getItemGraphURL = function(server, itemId) {
    var url = self.getTopURL(server);
    if (!url)
      return undefined;

    url += "history.php?action=showgraph&itemid=" + itemId + "&itemids%5B%5D=" + itemId;
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
