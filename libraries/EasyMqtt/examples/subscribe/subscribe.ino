#include <EasyMqtt.h>

EasyMqtt mqtt;

void setup() {
  Serial.begin(115200);
  Serial.println();  

  mqtt.wifi("ssid", "pass");
  mqtt.mqtt("server", 1883, "user", "password");

  mqtt["write"] >> [](String value) {
    Serial.print("Incomming: ");
    Serial.println(value);
  };
}

void loop() {
  mqtt.loop();
}
