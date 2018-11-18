# EasyMqtt
Easy handling of Mqtt on esp8266

* Easy wifi configuration using web interface
* Easy mqtt configuration using web interface
* Easy configuration of mqtt endpoints
* Web based UI to see current values

## Examble usage EasyMqtt
```C++
#include <EasyMqtt.h>

EasyMqtt mqtt;

void setup() {
  mqtt.config().set("foo", "My Foo");

  mqtt["foo"] << [&](){
    return String(mqtt.config().get("foo", "default"));
  };
}

void loop() {
  mqtt.loop();
}

```
