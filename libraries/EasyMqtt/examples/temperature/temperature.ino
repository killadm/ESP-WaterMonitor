#include <EasyMqtt.h>
#include <DHT.h>

EasyMqtt mqtt;
DHT dht(D7, DHT22);

void setup() {
  dht.begin();
  mqtt.wifi("ssid", "pass");
  mqtt.mqtt("server", 1883, "user", "password");

  mqtt["temperature"] << [&]() {
    return dht.readTemperature();
  };

  mqtt["humidity"] << [&]() {
    return dht.readHumidity();
  };

}

void loop() {
  mqtt.loop();
}
