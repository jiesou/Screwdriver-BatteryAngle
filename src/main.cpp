#include <DNSServer.h>
#ifdef ESP32
#include <WiFi.h>
#include <AsyncTCP.h>
#elif defined(ESP8266)
#include <ESP8266WiFi.h>
#include <ESPAsyncTCP.h>
#endif
#include "ESPAsyncWebServer.h"
#include "LittleFS.h" //我们把网页藏在LittleFS内
#include "ArduinoJson.h"
#include "ArduinoOTA.h"
#include "index_html.h"
#define CF1 13
#define BUTTON_PIN 2


class CurrentProcessor
{
private:
  static CurrentProcessor *instance;
  volatile unsigned long pulse_interval = 0;
  volatile unsigned int pulse_count = 0;
  volatile unsigned long last_pulse_time = 0;
  static void IRAM_ATTR CF1Interrupt()
  {
    unsigned long current_time = micros();
    instance->pulse_interval = current_time - instance->last_pulse_time;
    instance->last_pulse_time = current_time;
    instance->pulse_count++;
  }

public:
  volatile bool appliance_working = false;
  volatile bool btn_pressed = false;
  volatile unsigned int screw_count = 4;
  const int CF1_PIN = 13;
  const double FREQUENCY_THRESHOLD = 15.0;
  unsigned long last_update_time = 0;

  CurrentProcessor()
  {
    // 构造函数中保存实例指针
    instance = this;
  }

  void begin()
  {
    pinMode(14, OUTPUT);
    pinMode(BUTTON_PIN, INPUT_PULLUP);
    pinMode(CF1_PIN, INPUT);
    attachInterrupt(digitalPinToInterrupt(CF1_PIN), CurrentProcessor::CF1Interrupt, RISING);
  }

  void update()
  {
    unsigned long current_time = millis();
    if (current_time - last_update_time < 100) return; // 100ms更新一次
    last_update_time = current_time;

    
    unsigned int pulse_count_in_one_second = pulse_count * 10;
    pulse_count = 0;
    double frequency = 0;

    if (pulse_count_in_one_second != 0)
    {
      frequency = 1000000.0 / pulse_interval;
    }

    if (frequency > FREQUENCY_THRESHOLD && !appliance_working)
    {
      appliance_working = true;
      screw_count--;
      if (screw_count == 0)
      {
        screw_count = 4;
      }
    }
    else if (frequency <= FREQUENCY_THRESHOLD && appliance_working)
    {
      appliance_working = false;
    }

    btn_pressed = digitalRead(BUTTON_PIN) == LOW;
  }

  String getStatus()
  {
    return "{\"state\": " + String(pulse_count * 10) +
           ", \"frequency\": " + String(pulse_interval > 0 ? 1000000.0 / pulse_interval : 0) + "}";
  }
};

/* 电流解析处理 */
CurrentProcessor current_processor;
CurrentProcessor *CurrentProcessor::instance = nullptr; // 静态成员变量必须在类外部进行定义和初始化

DNSServer dnsServer;
AsyncWebServer server(80);

struct WiFiconfig
{
  String ssid;
  String password;
  bool config_renewed = false;
  bool connStatus = false;
};

WiFiconfig wificonfig;

class CaptiveRequestHandler : public AsyncWebHandler
{
public:
  CaptiveRequestHandler() {}
  virtual ~CaptiveRequestHandler() {}

  bool canHandle(AsyncWebServerRequest *request)
  {
    // request->addInterestingHeader("ANY");
    return true;
  }

  void handleRequest(AsyncWebServerRequest *request)
  {
    request->send(200, "text/html", index_html);
  }
};

// Initialize LittleFS
void initLittleFS()
{
  if (!LittleFS.begin())
  {
    Serial.println("An error has occurred while mounting LittleFS");
  }
  Serial.println("LittleFS mounted successfully");
}

// 保存 WiFi 配置到文件
void saveWiFiconfig(const String &ssid, const String &password)
{
  File file = LittleFS.open("/wifi_config.json", "w");
  if (!file)
  {
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
bool loadWiFiconfig()
{
  if (LittleFS.exists("/wifi_config.json"))
  {
    File file = LittleFS.open("/wifi_config.json", "r");
    if (file)
    {
      JsonDocument doc;
      DeserializationError error = deserializeJson(doc, file);
      if (error)
      {
        Serial.println("Failed to read file");
        return false;
      }

      // fixme
      wificonfig.ssid = doc["ssid"].as<String>();
      wificonfig.password = doc["password"].as<String>();

      file.close();
      Serial.println("WiFi Config loaded from /wifi_config.json");
      return true;
    }
    else
    {
      return false;
    }
  }
  else
  {
    Serial.println("No WiFi Config found, using default values");
    return false;
  }
}

// 在联网成功后，初始化ota
void initOTA()
{

  // Port defaults to 8266
  // ArduinoOTA.setPort(8266);

  // Hostname defaults to esp8266-[ChipID]
  // ArduinoOTA.setHostname("myesp8266");

  // No authentication by default
  // ArduinoOTA.setPassword("admin");

  // Password can be set with it's md5 value as well
  // MD5(admin) = 21232f297a57a5a743894a0e4a801fc3
  // ArduinoOTA.setPasswordHash("21232f297a57a5a743894a0e4a801fc3");

  ArduinoOTA.onStart([]()
                      {
  String type;
  if (ArduinoOTA.getCommand() == U_FLASH) {
    type = "sketch";
  } else {  // U_FS
    type = "filesystem";
  }

  // NOTE: if updating FS this would be the place to unmount FS using FS.end()
  Serial.println("Start updating " + type); });
  ArduinoOTA.onEnd([]()
                    { Serial.println("\nEnd"); });
  ArduinoOTA.onProgress([](unsigned int progress, unsigned int total)
                        { Serial.printf("Progress: %u%%\r", (progress / (total / 100))); });
  ArduinoOTA.onError([](ota_error_t error)
                      {
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
  } });
  ArduinoOTA.begin();
  Serial.println("OTA Ready");
}

void onWiFiEvent(WiFiEvent_t event)
{
  switch (event)
  {
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

void initSTA(String &ssid, String &password)
{
  // WiFi.disconnect(true);  // 清除配置并断开连接
  // delay(100);             // 等待一些时间确保清除完成
  WiFi.onEvent(onWiFiEvent); // 处理回调事件,这是一种esp8266老式的事件回调机制。有点不推荐所以黄线
  WiFi.begin(ssid, password);
}

// 设置初始化状态为SOFT_AP模式
void initSoftAP()
{
  WiFi.softAP("Socket");
  dnsServer.start(53, "*", WiFi.softAPIP());
  server.serveStatic("/", LittleFS, "/");

  // 由于我们使用了强制门户技术，为避免一些合理的请求也被重定向，因而要有一个过滤表
  server.addHandler(new CaptiveRequestHandler()).setFilter([](AsyncWebServerRequest *request)
                                                            {
  // 忽略 /toggle, /status, 和 /settings 请求
    String url = request->url();
    return !(url.startsWith("/toggle") || url.startsWith("/set_ap") || \
    url.startsWith("/settings")|| url.startsWith("/status")||url.startsWith("/get_config")); });
  // more handlers...
  server.on("/toggle", HTTP_GET, [](AsyncWebServerRequest *request)
            {
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
  } });

  server.on("/set_ap", HTTP_POST, [](AsyncWebServerRequest *request) {}, NULL, [](AsyncWebServerRequest *request, uint8_t *data, size_t len, size_t index, size_t total)
            {
    JsonDocument doc;
    deserializeJson(doc, data, len);

    String mode = doc["mode"];
    String ssid = doc["ssid"];
    String password = doc["password"];

    Serial.printf("Mode: %s, SSID: %s, Password: %s\n", mode.c_str(), ssid.c_str(), password.c_str());

    // Perform action based on the selected mode (e.g., configure WiFi)

    //由网页过来的信息自然要送给全局变量
    wificonfig.ssid = ssid;
    wificonfig.password = password;

    wificonfig.config_renewed = true;
    // Send response
    JsonDocument response;
    response["message"] = "AP mode and credentials set successfully!";
    String responseBody;
    serializeJson(response, responseBody);
    request->send(200, "application/json", responseBody); });

  server.on("/status", HTTP_GET, [](AsyncWebServerRequest *request)
            {
  JsonDocument response;
  response["sta_connected"] = wificonfig.connStatus;
  response["sta_error"] = !wificonfig.connStatus;
  response["ip"] = WiFi.localIP().toString();
  response["is_working"] = current_processor.appliance_working;
  response["btn_pressed"] = current_processor.btn_pressed;
  String responseBody;
  serializeJson(response, responseBody);
  request->send(200, "application/json", responseBody); });

  server.on("/get_config", HTTP_GET, [](AsyncWebServerRequest *request)
            {
  File file = LittleFS.open("/wifi_config.json", "r");
  if (!file) {
    request->send(500, "application/json", "{\"error\":\"Failed to open file\"}");
    return;
  }

  JsonDocument json;
  DeserializationError error = deserializeJson(json, file);
  file.close();

  if (error) {
    request->send(500, "application/json", "{\"error\":\"Failed to parse JSON\"}");
    return;
  }

  String response;
  serializeJson(json, response);
  request->send(200, "application/json", response); });

  server.begin();
}

void setup()
{
  Serial.begin(74880);
  initLittleFS();
  WiFi.mode(WIFI_AP_STA); // 只有这一个工作模式，STA有成功连上或者没连上的分别但是AP永远开着
  // your other setup stuff...
  initSoftAP(); // 初始化首先默认进入强制门户模式
  if (loadWiFiconfig() == true)
  {
    // 如果爱littlefs上读出了ssid和password那么就尝试连接
    wificonfig.config_renewed = true;
  }

  current_processor.begin();
}

void loop()
{
  dnsServer.processNextRequest(); // 强制门户runtime
  if (wificonfig.config_renewed == true)
  {
    wificonfig.config_renewed = false;
    initSTA(wificonfig.ssid, wificonfig.password);
  }
  if (wificonfig.connStatus == true)
    ArduinoOTA.handle(); // OTA服务runtime

  current_processor.update(); // 电流解析处理runtime
}
