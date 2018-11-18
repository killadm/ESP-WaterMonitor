# ToDo:

## Improvements

### Add easyMqtt/$id/system/connected + will to indicate if device is online

### OTA support
Add support for OTA using ArduinoOTA

## Future

### Support writing to a topic with a subscriber
Support both << and >> on the same topic with out read out own published values

### Add support for float type (extend Entry)
This will make it possible to generate graphs in UI and ease implementations

### Add publish configured endpoints, to support openhab2 auto configure
something like
easyMqtt/$id/system/config
{
  "id"="1dfasfa",
  "ip"="192.168.1.79",
  "endpoints"=["/temparatur", "/humidity"]
}

   Read / Write / String / Number / ...

