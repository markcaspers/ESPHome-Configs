#include "esphome.h"

class JuraCoffee : public PollingComponent, public UARTDevice {
 Sensor *xsensor1 {nullptr};
 Sensor *xsensor2 {nullptr};
 Sensor *xsensor3 {nullptr};
 Sensor *xsensor4 {nullptr};
 Sensor *xsensor5 {nullptr};
 TextSensor *xsensor6 {nullptr};
 TextSensor *xsensor7 {nullptr};

 public:
  JuraCoffee(UARTComponent *parent, Sensor *sensor1, Sensor *sensor2, Sensor *sensor3, Sensor *sensor4, Sensor *sensor5, TextSensor *sensor6, TextSensor *sensor7) : UARTDevice(parent) , xsensor1(sensor1) , xsensor2(sensor2) , xsensor3(sensor3) , xsensor4(sensor4) , xsensor5(sensor5) , xsensor6(sensor6) , xsensor7(sensor7){}

  long num_espresso, num_coffee, num_cappuccino, num_clean, num_coffee_since_clean;
  std::string tray_status, tank_status;

  // Jura communication function taken in entirety from cmd2jura.ino, found at https://github.com/hn/jura-coffee-machine
  String cmd2jura(String outbytes) {
    String inbytes;
    int w = 0;

    while (available()) {
      read();
    }

    outbytes += "\r\n";
    for (int i = 0; i < outbytes.length(); i++) {
      for (int s = 0; s < 8; s += 2) {
        char rawbyte = 255;
        bitWrite(rawbyte, 2, bitRead(outbytes.charAt(i), s + 0));
        bitWrite(rawbyte, 5, bitRead(outbytes.charAt(i), s + 1));
        write(rawbyte);
      }
      delay(8);
    }

    int s = 0;
    char inbyte;
    while (!inbytes.endsWith("\r\n")) {
      if (available()) {
        byte rawbyte = read();
        bitWrite(inbyte, s + 0, bitRead(rawbyte, 2));
        bitWrite(inbyte, s + 1, bitRead(rawbyte, 5));
        if ((s += 2) >= 8) {
          s = 0;
          inbytes += inbyte;
        }
      } else {
        delay(10);
      }
      if (w++ > 500) {
        return "";
      }
    }

    return inbytes.substring(0, inbytes.length() - 2);
  }

  void setup() override {
    this->set_update_interval(15000); // 600000 = 10 minutes // Now 15 seconds
  }

  void loop() override {
  }

  void update() override {
    String result, substring, StringTank, StringTray;
    int trayBit, tankBit;

    // Fetch our line of EEPROM
    result = cmd2jura("RT:0000");
    
    // For Testing
    ESP_LOGD("main", "Raw RT result: %s", result.c_str());
    
    // Get Espressos made
    substring = result.substring(7,11);
    num_espresso = strtol(substring.c_str(),NULL,16);
    
    // For Testing
    //ESP_LOGD("main", "Espresso result: %s", num_espresso);

    // Double Espressos made
    //substring = result.substring(7,11);
    //num_double_espresso = strtol(substring.c_str(),NULL,16);

    // Coffees made
    substring = result.substring(11,15);
    num_coffee = strtol(substring.c_str(),NULL,16);
    
    // For Testing
    //ESP_LOGD("main", "Coffee result: %s", num_coffee);

    // Double Coffees made
    //substring = result.substring(15,19);
    //num_double_coffee = strtol(substring.c_str(),NULL,16);

    // Cappuccinos made
    substring = result.substring(15,19);
    num_cappuccino = strtol(substring.c_str(),NULL,16);
    
    // For Testing
    //ESP_LOGD("main", "Cappuccino result: %s", num_cappuccino);

    // Cleanings done
    substring = result.substring(35,39);
    num_clean = strtol(substring.c_str(),NULL,16);
    
    // For Testing
    //ESP_LOGD("main", "Clean result: %s", num_clean);
    
    // Coffees/Espressos made since last cleaning
    substring = result.substring(63,67);
    num_coffee_since_clean = strtol(substring.c_str(),NULL,16);
    
    // For Testing
    //ESP_LOGD("main", "# Products since clean result: %s", num_coffee_since_clean);

    // Tray & water tank status
    // Much gratitude to https://www.instructables.com/id/IoT-Enabled-Coffee-Machine/ for figuring out how these bits are stored
    
    result = cmd2jura("IC:");
    StringTray = result.substring(3,4);
    StringTank = result.substring(5,6);
    trayBit = atoi(StringTray.c_str());
    tankBit = atoi(StringTank.c_str());
    if (trayBit == 4) { tray_status = "Plaats Lade"; } else if (trayBit == 5) { tray_status = "OK"; } else { tray_status = "Onbekend"; }
    if (tankBit == 2) { tank_status = "Vul Waterreservoir"; } else if (tankBit == 0) { tank_status = "OK"; } else { tank_status = "Onbekend"; }

    // For Testing
    // ESP_LOGD("main", "Raw IC result: %s", result.c_str());
    // ESP_LOGD("main", "StringTray IC result: %s", StringTray.c_str());
    // ESP_LOGD("main", "StringTank IC result: %s", StringTank.c_str());
      

    if (xsensor1 != nullptr)   xsensor1->publish_state(num_espresso);
    if (xsensor2 != nullptr)   xsensor2->publish_state(num_coffee);
    if (xsensor3 != nullptr)   xsensor3->publish_state(num_cappuccino);
    if (xsensor4 != nullptr)   xsensor4->publish_state(num_clean);
    if (xsensor5 != nullptr)   xsensor5->publish_state(num_coffee_since_clean);
    if (xsensor6 != nullptr)   xsensor6->publish_state(tray_status);
    if (xsensor7 != nullptr)   xsensor7->publish_state(tank_status);

  }
};