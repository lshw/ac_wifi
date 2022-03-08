#include <FS.h>
extern "C" {
#include "user_interface.h"
}
#include "config.h"
#include "global.h"
String hostname = HOSTNAME;
#include "gpio.h"
#include "ota.h"
#include "wifi_client.h"
#include "httpd.h"
void init1() {
  save_nvram();
}
uint32_t t0 = 0;
void setup()
{
  ESP.wdtEnable(50000);
  Serial.begin(115200);
  gpio_setup();
  load_nvram(); //从esp8266的nvram载入数据
  wifi_country_t mycountry =
  {
    .cc = "CN",
    .schan = 1,
    .nchan = 13,
    .policy = WIFI_COUNTRY_POLICY_MANUAL,
  };

  wifi_set_country(&mycountry);
  wifi_station_connect();
  pinMode(LEDP, OUTPUT);

  analogWrite(5, 5);
  play("12");//234567A"); //滴～～
  analogWrite(5, 0);

  send(0x2f0000L);
  send(0x2f0000L);
  nvram.boot_count++;
  nvram.change = 1;
  init1();
#ifdef GIT_COMMIT_ID
  Serial.println(F("Git Ver=" GIT_COMMIT_ID));
#endif
  Serial.print(F("SDK Ver="));
  Serial.println(ESP.getSdkVersion());

  Serial.print("Software Ver=" VER "\r\nBuildtime=");
  Serial.print(__YEAR__);
  Serial.write('-');
  if (__MONTH__ < 10) Serial.write('0');
  Serial.print(__MONTH__);
  Serial.write('-');
  if (__DAY__ < 10) Serial.write('0');
  Serial.print(__DAY__);
  Serial.println(F(" " __TIME__));
  hostname += String(ESP.getChipId(), HEX);
  WiFi.hostname(hostname);
  Serial.println("Hostname: " + hostname);
  Serial.flush();

  _myTicker.attach(1, timer1s);
  Serial.flush();
  wdt_disable();
  wifi_setup();
  if (wifi_station_get_connect_status() != STATION_GOT_IP) {
    ap_on_time = millis() + 30000;  //WPS 20秒
    if (WiFi.beginWPSConfig()) {
      delay(1000);
      uint8_t ap_id = wifi_station_get_current_ap_id();
      char wps_ssid[33], wps_password[65];
      memset(wps_ssid, 0, sizeof(wps_ssid));
      memset(wps_password, 0, sizeof(wps_password));
      struct station_config config[5];
      wifi_station_get_ap_info(config);
      strncpy(wps_ssid, (char *)config[ap_id].ssid, 32);
      strncpy(wps_password, (char *)config[ap_id].password, 64);
      config[ap_id].bssid_set = 1; //同名ap，mac地址不同
      wifi_station_set_config(&config[ap_id]); //保存成功的ssid,用于下次通讯
      wifi_set_add(wps_ssid, wps_password);
    }
    ESP.wdtFeed();
  }
  if (wifi_station_get_connect_status() != STATION_GOT_IP) {
    AP();
    ota_status = 1;
    httpd_listen();
    ota_setup();
  }
  ESP.wdtEnable(5000);
}

void wput() {
  uint16_t httpCode = wget();
}

bool httpd_up = false;
uint32_t last_check_connected;
bool last_keygen;
void loop()
{
  if (last_keygen != digitalRead(KEYWORD)) {
    last_keygen = digitalRead(KEYWORD);
    if (last_keygen == LOW) {
      if (keydown_ms + 20 > millis()) return;
      keydown_ms = millis();
      if (digitalRead(SSD) == HIGH) {
        Serial.println("down");
        digitalWrite(SSD, LOW);
        analogWrite(5, 5);
        play("2");
        analogWrite(5, 0);
      } else {
        Serial.println("up");
        digitalWrite(SSD, HIGH);
        analogWrite(5, 5);
        play("1");
        analogWrite(5, 0);
      }
    }

  }
  ESP.wdtFeed();
  last_check_connected = millis() + 1000; //1秒检查一次connected;
  if (ap_client_linked || connected_is_ok) {
    httpd_loop();
    ArduinoOTA.handle();
  }
  if (ap_client_linked)
    dnsServer.processNextRequest();
  if (connected_is_ok) {
    if (!httpd_up) {
      wput();
      httpd_up = true;
      httpd_listen();
    }
  }
  yield();
  if (nvram.change) save_nvram();
  system_soft_wdt_feed (); //各loop里要根据需要执行喂狗命令
}

bool smart_config() {
  //插上电， 等20秒， 如果没有上网成功， 就会进入 CO xx计数， 100秒之内完成下面的操作
  //手机连上2.4G的wifi,然后微信打开网页：http://wx.ai-thinker.com/api/old/wifi/config
  nvram.change = 1;
  if (wifi_connected_is_ok()) return true;
  WiFi.mode(WIFI_STA);
  WiFi.beginSmartConfig();
  Serial.println("SmartConfig start");
  for (uint8_t i = 0; i < 100; i++) {
    if (WiFi.smartConfigDone()) {
      wifi_set_clean();
      wifi_set_add(WiFi.SSID().c_str(), WiFi.psk().c_str());
      Serial.println("OK");
      return true;
    }
    Serial.write('.');
    delay(1000);
  }
  return false;
}
