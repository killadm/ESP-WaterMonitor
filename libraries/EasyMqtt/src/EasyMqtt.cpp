#include "EasyMqtt.h"

#include <ESP8266WiFi.h>
#include <PubSubClient.h>
#include "Entry.h"
#include "Config.h"
#include "WebPortal.h"
#include <ArduinoOTA.h>


/**
  * Handle connections to mqtt
  */
void EasyMqtt::connectWiFi() {
  if(WiFi.status() != WL_CONNECTED) {
    debug("Connecting to wifi");
    WiFi.mode(WIFI_STA);
    WiFi.hostname("ESP-WaterMonitor");
    WiFi.begin(config().get("wifi.ssid", ""), config().get("wifi.password", ""));

#ifdef DEBUG
    WiFi.printDiag(Serial);
#endif

    int timer = 0;
    while ((WiFi.status() == WL_DISCONNECTED) && timer < 60) {
      debug("Wifi Status", WiFi.status());
      delay(500);
      timer++;
    }
    if(timer < 60 && WiFi.status() == WL_CONNECTED) {
      debug("WiFi connected");
      debug("IP address", WiFi.localIP().toString());
    } else {
      debug("WiFi connection timeout - Setup AP");
      WiFi.mode(WIFI_AP);
      WiFi.softAP("EasyMqtt", "123456");
      debug("IP address", WiFi.softAPIP().toString());
    }
    debug("devideId", deviceId);
    webPortal.setup(this, &config(), &ntp());
  }
}

void EasyMqtt::disconnectWiFi() {
  if(WiFi.status() == WL_CONNECTED) {
    WiFi.softAPdisconnect();
    WiFi.disconnect();
    int timeout = 0;
    while(WiFi.status() != WL_DISCONNECTED && timeout < 200){
      delay(10);
      timeout++;
    }
  }
}

void EasyMqtt::connectMqtt() {
  if (!mqttClient.connected() && mqttDelay < millis()) {
    debug("Connecting to MQTT");
    mqttClient.setClient(wifiClient);
    mqttClient.setCallback([&](const char* topic, uint8_t* payload, unsigned int length) {
      each([=](Entry* entry){
        entry->callback(topic, payload, length);
      });
    });

    mqttClient.setServer(config().get("mqtt.host", ""), config().getInt("mqtt.port", 1883));

    if (mqttClient.connect(config().get("device.name", String(ESP.getChipId()).c_str()), config().get("mqtt.username", ""), config().get("mqtt.password", ""), get("$system")["online"].getTopic().c_str(), 1, 1, "OFF")) {
      debug("Connected to MQTT");

      setPublishFunction([&](Entry* entry, String message){
        if(mqttClient.connected()) {
        mqttClient.publish(entry->getTopic().c_str(), message.c_str(), true);
        }
      });

      debug("Topic", getTopic()); 

      each([&](Entry* entry){
        if (entry->isOut()) {
          mqttClient.subscribe(entry->getTopic().c_str());
        }
      });
      mqttDelay = 0;
    } else {
      debug("Connection to MQTT failed, rc", mqttClient.state());

      mqttDelay = millis() + 5000;
    }
  }
}

EasyMqtt::EasyMqtt() : Entry("easyMqtt") {
#ifdef DEBUG
  Serial.begin(115200);
#endif

  // Add config entry
  cfg = new Config();
  cfg->load();

  // Add time(NTP)
  ntpClient = new NTPClient();

  deviceId = config().get("device.name", String(ESP.getChipId()).c_str());

  char * password = config().get("password", "");
  if(strlen(password) > 0) {
    ArduinoOTA.setPassword(password);
  }
  ArduinoOTA.setHostname("ESP-WaterMoniter");
  ArduinoOTA.begin();
  
  setInterval(60, 10);

  get("$system").setInterval(300); // every 5 min
  get("$system")["deviceId"] << [this]() {
    return deviceId;
  };
  get("$system")["mem"]["heap"] << []() {
    return ESP.getFreeHeap();
  };
  get("$system")["uptime"] << []() {
    return millis() / 1000;
  };
  get("$system")["wifi"]["rssi"] << []() {
    return WiFi.RSSI();
  };
  get("$system")["wifi"]["quality"] << []() {
    int quality = (WiFi.RSSI()+100)*1.6667;
    if(quality < 0) quality = 0;
    if(quality > 100) quality = 100;
    return quality;
  };
  get("$system")["wifi"]["ssid"] << []() {
    return WiFi.SSID();
  };
  get("$system")["wifi"]["ip"] << []() {
    return WiFi.localIP().toString();
  };
  get("$system")["online"] << []() {
    return "ON";
  };

  get("$system")["time"].setInterval(900); // every 15 min
  get("$system")["time"] << [this]() {
    ntp().update();
    return ntp().getTime();
  };

  get("$system")["restart"] >> [this](String value) {
    if(value == config().get("password", "")) {
      debug("Restart");
      ESP.restart();
    }
  };

  get("$system")["reset"] >> [this](String value) {
    if(value == config().get("password", "")) {
      config().reset();
    }
  };
}

String EasyMqtt::getDeviceId() {
  return config().get("device.name", String(ESP.getChipId()).c_str());
}

String EasyMqtt::getTopic() {
  return config().get("device.name", String(ESP.getChipId()).c_str()) + String("/") +String(ESP.getChipId());
}

Config & EasyMqtt::config() {
  return *cfg;
}

NTPClient & EasyMqtt::ntp() {
  return *ntpClient;
}

/**
  Configure wifi
  Deprecated
  */
void EasyMqtt::wifi(const char* ssid, const char* password) {
  config().get("wifi.ssid", ssid);
  config().get("wifi.password", password);
}

/**
  Configure mqtt
  Deprecated
  */
void EasyMqtt::mqtt(const char* host, int port, const char* username, const char* password) {
  config().get("mqtt.host", host);
  config().get("mqtt.port", String(port).c_str());
  config().get("mqtt.username", username);
  config().get("mqtt.password", password);
}

/**
  Handle the normal loop
  */
void EasyMqtt::loop() {
  connectWiFi();
  ArduinoOTA.handle();
  if(WiFi.status() == WL_CONNECTED) {
    connectMqtt();
    if(mqttClient.connected()) {
      mqttClient.loop();
    }
    webPortal.loop();
    each([](Entry* entry){
      entry->update();
    });
  }
}
