#include <DNSServer.h>
#ifdef ESP32
#include <AsyncTCP.h>
#include <WiFi.h>
#elif defined(ESP8266)
#include <ESP8266WiFi.h>
#include <ESPAsyncTCP.h>
#endif
#include "ArduinoJson.h"
#include "ArduinoOTA.h"
#include "CurrentProcessor.h"
#include "ESPAsyncWebServer.h"
#include "LittleFS.h" //我们把网页藏在LittleFS内
#include "index_html.h"

/* 电流解析处理 */
CurrentProcessor current_processor;

DNSServer dnsServer;
AsyncWebServer server(80);
// 用于处理 status api http 流的全局变量
AsyncEventSource *status_stream_events = new AsyncEventSource("/status");

struct WiFiconfig {
  String ssid;
  String password;
  bool config_renewed = false;
  bool connStatus = false;
};

WiFiconfig wificonfig;

class CaptiveRequestHandler : public AsyncWebHandler {
public:
  CaptiveRequestHandler() {}
  virtual ~CaptiveRequestHandler() {}

  bool canHandle(AsyncWebServerRequest *request) {
    // request->addInterestingHeader("ANY");
    return true;
  }

  void handleRequest(AsyncWebServerRequest *request) {
    request->send(200, "text/html", index_html);
  }
};

// Initialize LittleFS
void initLittleFS() {
  if (!LittleFS.begin()) {
    Serial.println("An error has occurred while mounting LittleFS");
  }
  Serial.println("LittleFS mounted successfully");
}

// 保存 WiFi 配置到文件
void saveWiFiconfig(const String &ssid, const String &password) {
  File file = LittleFS.open("/wifi_config.json", "w");
  if (!file) {
    Serial.println("Failed to open file for writing");
    return;
  }

  JsonDocument doc;
  // 更新指定的配置
  doc["ssid"] = wificonfig.ssid;
  doc["password"] = wificonfig.password;

  serializeJson(doc, file);
  file.close();
  Serial.println("WiFi Config saved to /wifi_config.json");
}

// 从文件加载 WiFi 配置
bool loadWiFiconfig() {
  if (LittleFS.exists("/wifi_config.json")) {
    File file = LittleFS.open("/wifi_config.json", "r");
    if (file) {
      JsonDocument doc;
      DeserializationError error = deserializeJson(doc, file);
      if (error) {
        Serial.println("Failed to read file");
        return false;
      }

      // fixme
      wificonfig.ssid = doc["ssid"].as<String>();
      wificonfig.password = doc["password"].as<String>();

      file.close();
      Serial.println("WiFi Config loaded from /wifi_config.json");
      return true;
    } else {
      return false;
    }
  } else {
    Serial.println("No WiFi Config found, using default values");
    return false;
  }
}

// 在联网成功后，初始化ota
void initOTA() {
  // Port defaults to 8266
  // ArduinoOTA.setPort(8266);

  // Hostname defaults to esp8266-[ChipID]
  // ArduinoOTA.setHostname("myesp8266");

  // No authentication by default
  // ArduinoOTA.setPassword("admin");

  // Password can be set with it's md5 value as well
  // MD5(admin) = 21232f297a57a5a743894a0e4a801fc3
  // ArduinoOTA.setPasswordHash("21232f297a57a5a743894a0e4a801fc3");

  ArduinoOTA.onStart([]() {
    String type;
    if (ArduinoOTA.getCommand() == U_FLASH) {
      type = "sketch";
    } else { // U_FS
      type = "filesystem";
    }

    // NOTE: if updating FS this would be the place to unmount FS using FS.end()
    Serial.println("Start updating " + type);
  });
  ArduinoOTA.onEnd([]() { Serial.println("\nEnd"); });
  ArduinoOTA.onProgress([](unsigned int progress, unsigned int total) {
    Serial.printf("Progress: %u%%\r", (progress / (total / 100)));
  });
  ArduinoOTA.onError([](ota_error_t error) {
    Serial.printf("Error[%u]: ", error);
    if (error == OTA_AUTH_ERROR) {
      Serial.println("Auth Failed");
    } else if (error == OTA_BEGIN_ERROR) {
      Serial.println("Begin Failed");
    } else if (error == OTA_CONNECT_ERROR) {
      Serial.println("Connect Failed");
    } else if (error == OTA_RECEIVE_ERROR) {
      Serial.println("Receive Failed");
    } else if (error == OTA_END_ERROR) {
      Serial.println("End Failed");
    }
  });
  ArduinoOTA.begin();
  Serial.println("OTA Ready");
}

void onWiFiEvent(WiFiEvent_t event) {
  switch (event) {
  case WIFI_EVENT_STAMODE_GOT_IP:
    Serial.println("Ready");
    Serial.print("IP address: ");
    Serial.println(WiFi.localIP());
    wificonfig.connStatus = true;
    saveWiFiconfig(wificonfig.ssid, wificonfig.password);
    initOTA(); // 在连接良好的情况下启动初始化OTA服务
    break;
  case WIFI_EVENT_STAMODE_DISCONNECTED:
    wificonfig.connStatus = false;
    Serial.println("Disconnected from Wi-Fi.");
    break;
  default:
    break;
  }
}

void initSTA(String &ssid, String &password) {
  // WiFi.disconnect(true);  // 清除配置并断开连接
  // delay(100);             // 等待一些时间确保清除完成
  WiFi.onEvent(
      onWiFiEvent); // 处理回调事件,这是一种esp8266老式的事件回调机制。有点不推荐所以黄线
  WiFi.begin(ssid, password);
}

// 设置初始化状态为SOFT_AP模式
void initSoftAP() {
  WiFi.softAP("Socket");
  dnsServer.start(53, "*", WiFi.softAPIP());
  server.serveStatic("/", LittleFS, "/");

  // 由于我们使用了强制门户技术，为避免一些合理的请求也被重定向，因而要有一个过滤表
  server.addHandler(new CaptiveRequestHandler())
      .setFilter([](AsyncWebServerRequest *request) {
        // 忽略 /toggle, /status, 和 /settings 请求
        String url = request->url();
        return !(url.startsWith("/toggle") || url.startsWith("/set_ap") ||
                 url.startsWith("/settings") || url.startsWith("/status") ||
                 url.startsWith("/get_config"));
      });

  // stream_event 只需添加一次 handler
  server.addHandler(status_stream_events);

  // more handlers...
  server.on("/toggle", HTTP_GET, [](AsyncWebServerRequest *request) {
    if (request->hasParam("state")) {
      String state = request->getParam("state")->value();

      // 打印复选框状态或执行相应动作
      Serial.print("Checkbox state: ");
      Serial.println(state);

      // 根据状态执行动作，例如控制LED、继电器等
      // if (state == "on") {
      //     // 打开LED或继电器
      // } else {
      //     // 关闭LED或继电器
      // }

      request->send(200, "text/plain", "Checkbox state updated");
    } else {
      request->send(400, "text/plain", "State parameter missing");
    }
  });

  server.on(
      "/set_ap", HTTP_POST, [](AsyncWebServerRequest *request) {}, NULL,
      [](AsyncWebServerRequest *request, uint8_t *data, size_t len,
         size_t index, size_t total) {
        JsonDocument doc;
        deserializeJson(doc, data, len);

        String mode = doc["mode"];
        String ssid = doc["ssid"];
        String password = doc["password"];

        Serial.printf("Mode: %s, SSID: %s, Password: %s\n", mode.c_str(),
                      ssid.c_str(), password.c_str());

        // Perform action based on the selected mode (e.g., configure WiFi)

        // 由网页过来的信息自然要送给全局变量
        wificonfig.ssid = ssid;
        wificonfig.password = password;

        wificonfig.config_renewed = true;
        // Send response
        JsonDocument response;
        response["message"] = "AP mode and credentials set successfully!";
        String responseBody;
        serializeJson(response, responseBody);
        request->send(200, "application/json", responseBody);
      });

  server.on("/status", HTTP_GET, [](AsyncWebServerRequest *request) {
    // 连接处理仅记录连接事件
    status_stream_events->onConnect([](AsyncEventSourceClient *client) {
      if (client->lastId()) {
        Serial.printf("Client reconnected! Last message ID: %u\n",
                      client->lastId());
      }
    });
    request->send(200, "text/event-stream", "data: {}");
  });

  server.on("/get_config", HTTP_GET, [](AsyncWebServerRequest *request) {
    File file = LittleFS.open("/wifi_config.json", "r");
    if (!file) {
      request->send(500, "application/json",
                    "{\"error\":\"Failed to open file\"}");
      return;
    }

    JsonDocument json;
    DeserializationError error = deserializeJson(json, file);
    file.close();

    if (error) {
      request->send(500, "application/json",
                    "{\"error\":\"Failed to parse JSON\"}");
      return;
    }

    String response;
    serializeJson(json, response);
    request->send(200, "application/json", response);
  });

  server.begin();
}

void setup() {
  Serial.begin(74880);
  initLittleFS();
  WiFi.mode(
      WIFI_AP_STA); // 只有这一个工作模式，STA有成功连上或者没连上的分别但是AP永远开着
  // your other setup stuff...
  initSoftAP(); // 初始化首先默认进入强制门户模式
  if (loadWiFiconfig() == true) {
    // 如果爱littlefs上读出了ssid和password那么就尝试连接
    wificonfig.config_renewed = true;
  }

  current_processor.begin();
  tone(12, 1000);
}

void loop() {
  dnsServer.processNextRequest(); // 强制门户runtime
  if (wificonfig.config_renewed == true) {
    wificonfig.config_renewed = false;
    initSTA(wificonfig.ssid, wificonfig.password);
  }
  if (wificonfig.connStatus == true)
    ArduinoOTA.handle(); // OTA服务runtime

  current_processor.update(); // 电流解析处理runtime

  static unsigned long last_pushupdate_time = 0;
  if (millis() - last_pushupdate_time > 100) { // 每100ms更新一次
    JsonDocument doc;
    doc["sta_connected"] = wificonfig.connStatus;
    doc["sta_error"] = !wificonfig.connStatus;
    doc["ip"] = WiFi.localIP().toString();
    doc["frequency"] = current_processor.frequency;
    doc["btn_pressed"] = current_processor.btn_pressed;

    String output;
    serializeJson(doc, output);

    Serial.println(status_stream_events->count());

    // 向所有连接的客户端发送更新
    if (status_stream_events->count() > 0) { // 如果有连接的客户端
      status_stream_events->send(output.c_str());
    }

    last_pushupdate_time = millis();
  }
}
