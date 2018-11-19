# ESP-WaterMonitor
基于esp8266的TDS、流量监测系统

![](http://img.killadm.com/18-11-19/92132411.jpg)

## 特性

- 双路 TDS/温度监测
- 双路流速/流量监测
- WEB界面展示当前值/更新时间
- MQTT/RESTful API支持
- Arduino OTA/WEB OTA支持
- 基于telnet的远程debug输出
- 提供HomeAassistant配置范例

## 依赖

- 此项目依赖EasyMqtt库 https://github.com/bloft/EasyMqtt

- 在libraries/EasyMqtt目录下有一个汉化版本

## 硬件

- BA01 双通道TDS检测模块

  ![](http://img.killadm.com/18-11-18/41525627.jpg)

| 检测参数 |      误差      | 工作电压 | 接口 | 检测通道 |
| :------: | :------------: | :------: | :--: | :------: |
| TDS/水温 | <2%F.S./±0.5°C |  3.3 V   | UART |  双通道  |

注意：TDS探头规格为3分，普通净水器水管为2分，需要两个232三通做转接

![](http://img.killadm.com/18-11-19/89430904.jpg)



- YF-S402 霍尔流量计

![](http://img.killadm.com/18-11-18/40809858.jpg)

| 工作电压 | 允许耐压  | 流量范围 | 输出波形 | 接口 |
| :------: | :-------: | :------: | :------: | :--: |
|    5V    | < 1.75Mpa |  0.3-5L  |   方波   | 2分  |

连接方式：

​              红  IN   接正极 
​              黄  OUT  信号输出线
​              黑  GND   接负极 

频率：F=73*Q(Q 为流量 L/min)  流完一升水输出 4380 个脉冲

- ESP8266

![](http://img.killadm.com/18-11-19/64619394.jpg)

## 连接

待更新



