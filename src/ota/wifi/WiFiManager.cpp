#include "ota/wifi/WiFiManager.h"
#include "StoredConfig.h"

WiFiManager *WiFiManager::instance = nullptr;

WiFiManager::WiFiManager() : _isConnected(false) {
  instance = this;
  WiFi.mode(WIFI_AP_STA);
  WiFi.onEvent(onWiFiEvent);
}

void WiFiManager::startAP() {
  // 设置独一无二的 AP 名以区分
  String apName = "BatteryAngle_" + String((uint32_t)ESP.getChipId(), HEX);
  // 同时设置主机名，使其和AP名称一致
  WiFi.hostname(apName);
  WiFi.softAP(apName.c_str());
}

void WiFiManager::connectToWiFi(const String &ssid, const String &password) {
  WiFi.begin(ssid.c_str(), password.c_str());
}

bool WiFiManager::isConnected() const { return _isConnected; }

String WiFiManager::getLocalIP() const { return WiFi.localIP().toString(); }

void WiFiManager::onWiFiEvent(WiFiEvent_t event) {
  if (!instance)
    return; // 安全检查
  switch (event) {
  case WIFI_EVENT_STAMODE_GOT_IP:
    instance->_isConnected = true;
    if (instance->_onConnect)
      instance->_onConnect();
    break;

  case WIFI_EVENT_STAMODE_DISCONNECTED:
    instance->_isConnected = false;
    if (instance->_onDisconnect)
      instance->_onDisconnect();
    break;

  default:
    break;
  }
}
