#include <EasyMqtt.h>
#include <RemoteDebug.h>
#include <ESP8266WebServer.h>
#include <ESP8266mDNS.h>
#include <ESP8266HTTPUpdateServer.h>
#include <EEPROM.h>
#include "SoftwareSerial.h"

//WIFI设置
#define wifi_ssid "ASUS" // WIFI名称
#define wifi_pass "123456" // WIFI密码

//MQTT设置
#define mqtt_server "192.168.123.100" // MQTT服务器
#define mqtt_port 1883 // MQTT端口
#define mqtt_user "homeassistant" // MQTT用户名
#define mqtt_pass "123456" // MQTT密码

//OTA设置
const char* host = "ESP-WaterMonitor";
const char* update_path = "/"; // OTA页面地址
const char* update_username = "admin"; // OTA用户名
const char* update_password = "123456"; // OTA密码

SoftwareSerial mytds(5, 4); // RX, TX （D1, D2）接tds模块的TX和RX

byte flowSensorPin1 = 12; // D6 接流量计1黄色针脚
byte flowSensorPin2 = 14; // D5 接流量计2黄色针脚
byte ledPin = 1; // 状态指示灯

ESP8266WebServer httpServer(8266);
ESP8266HTTPUpdateServer httpUpdater;

RemoteDebug Debug;
EasyMqtt mqtt;

const int ProtocolHeaderByteTDS = 0xAA; // TDS协议头
const int ProtocolHeaderByteTemp = 0xAB; // 温度协议头
const int ProtocolHeaderByteError = 0xAC; // 异常消息协议头
const int ProtocolHeaderLength = 1; // TDS协议头长度
const int ProtocolBodyLength = 4; // TDS协议体长度
const int ProtocolChecksumLength = 1; // 校验值长度

bool appearToHaveValidMessage;
byte command[]={0xA0,0x00,0x00,0x00,0x00,0xA0}; // 查询指令
byte receivedMessage[ProtocolBodyLength];

int tds1 = 0;
int tds2 = 0;
float temp1 = 0.00;
float temp2 = 0.00;

// 流量计相关
bool reset1 = false;
bool reset2 = false;

// YF-S402 频率：F=73*Q(Q 为流量 L/min) 误差：±10 流完一升水输出 4380 个脉冲
float calibrationFactor1 = 4380;
float calibrationFactor2 = 4380;

float frequencyFactor = 73;

float flowRate1 = 0;
float flowRate2 = 0;

float totalFlow1 = 0.00;
float totalFlow2 = 0.00;

volatile byte pulseCount1 = 0;
volatile byte pulseCount2 = 0;

unsigned long oldTime = 0;
unsigned long previousTime = 0;

void setup() {
  // Serial.begin(115200);
  mytds.begin(9600);

  Debug.begin("Telnet_HostName");
  Debug.setResetCmdEnabled(true);

  mqtt.config().set("device.name", "ESP-WaterMonitor");
  mqtt.wifi(wifi_ssid, wifi_pass);
  mqtt.mqtt(mqtt_server, mqtt_port, mqtt_user, mqtt_pass);

  //OTA
  MDNS.begin(host);
  httpUpdater.setup(&httpServer, update_path, update_username, update_password);
  httpServer.begin();
  MDNS.addService("http", "tcp", 8266);
  Debug.printf("HTTPUpdateServer ready! Open http://%s.local%s in your browser and login with username '%s' and password '%s'\n", host, update_path, update_username, update_password);
  EEPROM.begin(512);

  if (!reset1) {
     EEPROM.get(100, totalFlow1);
     // 初始化EEPROM
     if (isnan(totalFlow1)) {
      totalFlow1 = 0.00;
     }
  }

  if (!reset2) {
     EEPROM.get(200, totalFlow2);
     if (isnan(totalFlow2)) {
      totalFlow2 = 0.00;
     }
  }

  pinMode(flowSensorPin1, INPUT);
  pinMode(flowSensorPin2, INPUT);
  pinMode(ledPin, OUTPUT);

  digitalWrite(flowSensorPin1, HIGH);
  digitalWrite(flowSensorPin2, HIGH);

  attachInterrupt(flowSensorPin1, pulseCounter1, FALLING);
  attachInterrupt(flowSensorPin2, pulseCounter2, FALLING);

  mqtt.setInterval(3);
  mqtt["totalFlow2"] << [](){ return totalFlow2; };
  mqtt["totalFlow1"] << [](){ return totalFlow1; };
  mqtt["flowRate2"] << [](){ return flowRate2; };
  mqtt["flowRate1"] << [](){ return flowRate1; };
  mqtt["temp2"] << [](){ return temp2; };
  mqtt["temp1"] << [](){ return temp1; };
  mqtt["tds2"] << [](){ return tds2; };
  mqtt["tds1"] << [](){ return tds1; };
}

// 把 2 byte数据转换为int
int hexToDec (int high, int low) {
  int z = 0;
  z = (high<<8)|low;
  return z;
}

void checkTDS() {

  if ((millis() - oldTime) > 3000) {

    byte ProtocolHeader = 0x00;
    // 发送查询指令
    mytds.write(command,6);
    int availableBytes = mytds.available();
    while (availableBytes>0) {

      if (!appearToHaveValidMessage) {

        // 寻找协议头
        if (availableBytes >= ProtocolHeaderLength) {

          // 读取1byte数据
          byte firstByte = mytds.read();

          if (firstByte == ProtocolHeaderByteTDS ||
              firstByte == ProtocolHeaderByteTemp ||
              firstByte == ProtocolHeaderByteError) {
              // 发现有效数据
              appearToHaveValidMessage = true;
              ProtocolHeader = firstByte;
              availableBytes = mytds.available();
          }
        }
        else
        {
           Debug.print("not found protocol header!");
        }
      }

      if (availableBytes >= (ProtocolBodyLength + ProtocolChecksumLength) && appearToHaveValidMessage) {

        // 读取协议体，添加协议头并计算校验和
        byte calculatedChecksum = 0x00;
        calculatedChecksum += ProtocolHeader;

        for (int i = 0; i < ProtocolBodyLength; i++) {
          receivedMessage[i] = mytds.read();
          calculatedChecksum += receivedMessage[i];
        }
        // 读取校验和
        byte receivedChecksum = mytds.read();
        //  对比校验和
        if (receivedChecksum == calculatedChecksum) {
          // 解析TDS数据
          if (ProtocolHeader == ProtocolHeaderByteTDS)
          {
            tds1 = hexToDec(receivedMessage[0],receivedMessage[1]);
            tds2 = hexToDec(receivedMessage[2],receivedMessage[3]);
            Debug.print("tds1:");
            Debug.println(tds1);
            Debug.print("tds2:");
            Debug.println(tds2);
          }
          // 解析温度数据数据
          if (ProtocolHeader == ProtocolHeaderByteTemp)
          {
            temp1 = hexToDec(receivedMessage[0],receivedMessage[1])/100.00;
            temp2 = hexToDec(receivedMessage[2],receivedMessage[3])/100.00;
            Debug.print("temp1:");
            Debug.println(temp1);
            Debug.print("temp2:");
            Debug.println(temp2);
          }
          // 异常代码： 01：命令帧异常
          //           02：忙碌中
          //           03：校正失败
          //           04：检测温度超出范围
          if (ProtocolHeader == ProtocolHeaderByteError)
          {
            Debug.print("error:");
            Debug.println(receivedMessage[0]);
          }
        } else {
          Debug.print("checksum error!receivedMessage:");
          Debug.println(receivedMessage[0]);
          Debug.println(receivedMessage[1]);
          Debug.println(receivedMessage[2]);
          Debug.println(receivedMessage[3]);
        }
      appearToHaveValidMessage = false;
      }
    // 模块每次发送12byte数据,包含 6 byte TDS 数据和 6 byte 温度数据，需要循环两次分别校验
    availableBytes -= 6;
    }
  oldTime = millis();
  }
}

void checkFlow() {

  unsigned long elapsedTime = millis() - previousTime;

  if (elapsedTime >= 1000) {
    // 计算前禁用中断
    detachInterrupt(flowSensorPin1);
    detachInterrupt(flowSensorPin2);

    // 计算流量
    float litresFlowed1 = pulseCount1 / calibrationFactor1;
    float litresFlowed2 = pulseCount2 / calibrationFactor2;

    // 计算脉冲频率
    float pulseFrequency1 = pulseCount1 * (1000.0 / elapsedTime);
    float pulseFrequency2 = pulseCount2 * (1000.0 / elapsedTime);

    // 计算流速(单位：L/min)
    float flowRate1 = pulseFrequency1 / frequencyFactor;
    float flowRate2 = pulseFrequency2 / frequencyFactor;

    // 把本次流量累加到总流量
    totalFlow1 += litresFlowed1;
    totalFlow2 += litresFlowed2;

    // 把总流量存入EEPROM
    EEPROM.put(100, totalFlow1);
    EEPROM.put(200, totalFlow2);
    EEPROM.commit();

    Debug.print("flowRate1:");
    Debug.println(flowRate1);

    Debug.print("flowRate2:");
    Debug.println(flowRate2);

    Debug.print("totalFlow1:");
    Debug.println(totalFlow1);

    Debug.print("totalFlow2:");
    Debug.println(totalFlow2);

    // 重置脉冲计数器
    pulseCount1 = 0;
    pulseCount2 = 0;

    previousTime = millis();

    // 恢复中断
    attachInterrupt(flowSensorPin1, pulseCounter1, FALLING);
    attachInterrupt(flowSensorPin2, pulseCounter2, FALLING);
  }
}

void pulseCounter1() {
  pulseCount1++;
}

void pulseCounter2() {
  pulseCount2++;
}

void loop() {
  Debug.handle();
  httpServer.handleClient();
  checkTDS();
  checkFlow();
  mqtt.loop();
  wdt_reset();
}
