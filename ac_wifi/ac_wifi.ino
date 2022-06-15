#include <FS.h>
extern "C" {
#include "user_interface.h"
}
#include "config.h"
#include "global.h"
#include "hlw8032.h"
#include "gpio.h"
#include "clock.h"
#include "ota.h"
#include "wifi_client.h"
#include "httpd.h"
uint32_t dida0 = 0;
uint8_t count_100ms = 0;
void run_20ms() {
  key_check();
  sound_20ms();
  ac_20ms();
  ac_decode();
  if (i_over > 20)
    i_over -= 20;
  else
    i_over = 0;
  if (millis() > dida0) {
    dida0 += 1000;
    sec();
  }
  count_100ms++;
  if (count_100ms >= 5) {
    count_100ms = 0;
    data100ms[data100ms_p] = power;
    data100ms_p = (data100ms_p + 1) % 600;
  }
}
uint32_t t0 = 0;
void setup()
{
  ESP.wdtEnable(50000);
  Serial.begin(4800, SERIAL_8E1); //hlw8032需要这个速度
  gpio_setup();
  load_nvram(); //从esp8266的nvram载入数据
  load_set(); //从files载入数据
  memset(&now, 0, sizeof(now));
  _myTicker.attach_ms(20, run_20ms);

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
  play("1"); //滴～～
  delay(1);
  led_send(sets.color);
  delay(1);
  led_send(sets.color);
  delay(1);
  save_nvram();
#ifdef GIT_VER
  Serial.println(F("Git Ver=" GIT_VER));
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
  hostname += String(sets.serial) + "_" + String(ESP.getChipId(), HEX);
  hostname.toUpperCase();
  Serial.println("Hostname: " + hostname);
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
uint32_t last_wput = 0;
void loop()
{
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
      play("3");
      httpd_up = true;
      httpd_listen();
      if (!ntp_get("ntp.anheng.com.cn"))
        ntp_get("1.debian.pool.ntp.org");
    }
    if (millis() > last_wput) {
      last_wput = millis() + 1000 * 3600 * 4; //4小时上传一次服务器
      wput();
    }
  }
  yield();
  system_soft_wdt_feed (); //各loop里要根据需要执行喂狗命令
  if (set_modi && (set_modi & SET_CHARGE)) {
    save_set(false); // 保存 /sets.txt
  }
  yield();
  if (time_update & DAY_UP) {
    day();
    time_update &= ~DAY_UP;
    yield();
  }
  if (time_update & HOUR_UP) {
    hour();
    time_update &= ~HOUR_UP;
    yield();
  }
  if (time_update & MIN_UP) {
    minute();
    time_update &= ~MIN_UP;
    yield();
  }
  system_soft_wdt_feed (); //各loop里要根据需要执行喂狗命令
  if (reboot_now) {
    nvram_save = millis();
    save_nvram_file();
    reboot_now = false;
    ESP.restart();
  }
  if (kwh_days_p == -1 && now.tm_year > 121) {
    load_kwh_days();
  }
}

void load_kwh_days() {
  File fp;
  kwh_days_p = 0;
  memset(kwh_days, 0, sizeof(kwh_days));
  if (SPIFFS.begin()) {
    String fn = "/" + String(now.tm_year + 1900 - 1) + ".dat";
    Serial.println(fn);
    if (SPIFFS.exists(fn)) {
      Serial.println("exists");
      fp = SPIFFS.open(fn, "r");
      if (fp) {
        Serial.println("open ok");
        while (fp.available()) {
          Serial.print("read()=");
          Serial.println(fp.read((uint8_t *)&kwh_days[kwh_days_p], sizeof(dataday)));
          kwh_days_p = (kwh_days_p + 1) % KWH_DAYS;
        }
        fp.close();
      }
    }
    fn = "/" + String(now.tm_year + 1900) + ".dat";
    Serial.println(fn);
    if (SPIFFS.exists(fn)) {
      Serial.println("exists");
      fp = SPIFFS.open(fn, "r");
      if (fp) {
        Serial.println("open ok");
        while (fp.available()) {
          Serial.print("read()=");
          Serial.println(fp.read((uint8_t *)&kwh_days[kwh_days_p], sizeof(dataday)));
          kwh_days_p = (kwh_days_p + 1) % KWH_DAYS;
        }
        fp.close();
      }
    }
    fn = "";
    SPIFFS.end();
  }
}
extern float datamins[60];//240 byte 每分钟最大功率
void minute() {
  datamins[now.tm_min] = 0.0;
  if ((now.tm_min % 10) == 0)
    save_nvram();
  if ((nvram_save > 0 && nvram_save <= millis())
      || (last_save + 120000 < millis())
      || last_save > millis())
    save_nvram_file();
  Serial.printf("%s\r\n", asctime(&now));
}
extern float datahour[24];//96字节  每一小时的耗电量
void hour() {
  datahour[now.tm_hour] = get_kwh() - nvram.kwh_hour0;
  nvram.kwh_hour0 = get_kwh();
  save_nvram();
  if (SPIFFS.begin()) {
    File fp;
    fp = SPIFFS.open("/hours.dat", "a");
    fp.write((char *) &datahour, sizeof(datahour));
    fp.close();
    fp = SPIFFS.open("/hours.dat", "r");
    SPIFFS.end();
  }
  loop_clock();
}
void day() {
  if (now.tm_year > 2021 - 1900) {
    kwh_days[kwh_days_p].kwh = get_kwh() - nvram.kwh_day0;
    kwh_days[kwh_days_p].time = mktime(&now);
    nvram.kwh_day0 = get_kwh();
    if (SPIFFS.begin()) {
      File fp;
      fp = SPIFFS.open("/" + String(now.tm_year + 1900) + ".dat", "a");
      fp.write((char *) &kwh_days[kwh_days_p], sizeof(dataday));
      fp.close();
      SPIFFS.end();
    }
    kwh_days_p = (kwh_days_p + 1 ) % KWH_DAYS;
  }
}
bool smart_config() {
  //插上电， 等20秒， 如果没有上网成功， 就会进入 CO xx计数， 100秒之内完成下面的操作
  //手机连上2.4G的wifi,然后微信打开网页：http://wx.ai-thinker.com/api/old/wifi/config
  save_nvram();
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
