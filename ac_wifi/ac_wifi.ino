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
void run_20ms() {
  key_check();
  sound_20ms();
  ac_20ms();
  ac_decode();
  if (i_over > 20)
    i_over -= 20;
  else
    i_over = 0;
}
uint32_t t0 = 0;
void setup()
{
  ESP.wdtEnable(50000);
  /*
    while (millis() < 2000) {
      ESP.wdtFeed();
      yield();
    }
  */
  Serial.begin(4800, SERIAL_8E1); //hlw8032需要这个速度
  gpio_setup();
  load_nvram(); //从esp8266的nvram载入数据
  load_set(); //从files载入数据
  setup_clock();
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
  send(0x2f0000L);
  send(0x2f0000L);
  nvram.boot_count++;
  save_nvram();
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
  hostname.toUpperCase();
  Serial.println("Hostname: " + hostname);
  Serial.flush();

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
uint32_t last_disp_time = 0;
bool httpd_up = false;
void loop()
{
  if (ssr_change & 0x80) {
    ssr_change &= ~0x80;
    if (ssr_change == 0) {
      Serial.println("OUT CLOSE");
      play("1");
    } else {
      Serial.println("OUT OPEN");
      play("2");
    }
  }
  if (nvram_save > 0 && nvram_save <= millis())
    save_nvram_file();
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
      wput();
      httpd_up = true;
      httpd_listen();
      if (!ntp_get("ntp.anheng.com.cn"))
        ntp_get("1.debian.pool.ntp.org");
      last_disp_time = 0;
    }
  }
  yield();
  system_soft_wdt_feed (); //各loop里要根据需要执行喂狗命令
  if (set_modi && (set_modi & SET_CHARGE)) {
    save_set();
  }
  loop_clock();
  if ((now.tm_sec == 0 && last_disp_time < millis()) || last_disp_time == 0) {
    last_disp_time = millis() + 60000 - now.tm_sec * 1000;
    Serial.printf("%s\r\n", asctime(&now));
  }
  yield();
  system_soft_wdt_feed (); //各loop里要根据需要执行喂狗命令
  if (reboot_now) {
    save_nvram_file();
    reboot_now = false;
    ESP.restart();
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
