#include <FS.h>
extern "C" {
#include "user_interface.h"
}
#include "config.h"
#include "global.h"
#include "netlog.h"
#include "hlw8032.h"
#include "gpio.h"
#include "clock.h"
#include "wifi_client.h"
#include "httpd.h"
uint32_t dida0 = 0;
uint8_t count_100ms = 0;
void run_20ms() {
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
void setup()
{
  ESP.wdtEnable(50000);
  Serial.begin(4800, SERIAL_8E1); //hlw8032需要这个速度
  load_set(); //从files载入数据
  gpio_setup();
  load_nvram(); //从esp8266的nvram载入数据
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
  play((char *) "1"); //滴～～
  delay(1);
  led_send(sets.color);
  delay(1);
  led_send(sets.color);
  delay(1);
  save_nvram();
#ifdef GIT_VER
  Serial.println(F("Git Ver=" GIT_VER));
#endif
  String hostname0 = String(ESP.getChipId(), HEX);
// 补零到8位
while (hostname0.length() < 6) {
  hostname0 = "0" + hostname0;
}
  hostname += String(sets.serial) + "-" + hostname0;
  hostname.toUpperCase();
  if (ac_name == "")
    ac_name = hostname;
  Serial.print(F("SDK Ver="));
  Serial.println(ESP.getSdkVersion());

  Serial.print(F("Software Ver=" VER "\r\nBuildtime="));
  Serial.print(__YEAR__);
  Serial.write('-');
  if (__MONTH__ < 10) Serial.write('0');
  Serial.print(__MONTH__);
  Serial.write('-');
  if (__DAY__ < 10) Serial.write('0');
  Serial.print(__DAY__);
  Serial.println(F(" " __TIME__));
  Serial.print(F("Hostname: "));
  Serial.println(ac_name);
  Serial.print(F("SN: "));
  Serial.println(hostname);
  Serial.flush();
  wifi_setup();
  ESP.wdtEnable(5000);
  Serial.printf(PSTR("空闲ram:%ld\r\n"), ESP.getFreeHeap());
}

bool httpd_up = false;
uint32_t last_wget = 0;
uint8_t smart_status = 0; //=0 smart未运行， =1 正在进行 尚未松开按键, =2 正在进行，已经松开按键, =3退出中， 检查松开就变成0
void loop()
{
  if (set0.relink) {
    set0.relink = false;
    wifi_setup();
    connected_is_ok = false;
  }
  if (wifi_connected_is_ok()) {
    if (!httpd_up) {
      play((char *) "23");
      httpd_up = true;
      httpd_listen();
      loop_clock(true);
    }
    httpd_loop();
    if (millis() > last_wget) {
      last_wget = millis() + 1000 * 3600 * 4; //4小时上传一次服务器
      wget();
    }
    yield();
    if(WiFi.status() != WL_CONNECTED) {
      set0.relink = true;
    }
  }
  system_soft_wdt_feed ();
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
  system_soft_wdt_feed ();
  if (set0.reboot_now) {
    Serial.println(F("reboot..."));
    Serial.flush();
    nvram_save = millis();
    save_nvram_file();
    set0.reboot_now = false;
    ESP.restart();
  }
  if (kwh_days_p == -1 && now.tm_year > 121) {
    load_kwh_days();
  }
  if ( smart_status == 0 && keydown_ms > 0 && millis() - keydown_ms > 5000 && digitalRead(KEYWORD) == LOW) {
    keydown_ms = 0;
    Serial.println(F("smart_config() begin"));
    smart_status = 1;
    smart_config();
    led_send(sets.color);
    smart_status = 3; //退出进行中
    Serial.println(F("smart_config() end"));
  }
  if (smart_status == 3  && digitalRead(KEYWORD)) { //等待松开按键就结束过程
    Serial.println(F("smart_config 结束"));
    smart_status = 0;
  }
#ifdef NETLOG
  netlog_loop();
#endif
}

void load_kwh_days() {
  File fp;
  kwh_days_p = 0;
  memset(kwh_days, 0, sizeof(kwh_days));
  if (SPIFFS.begin()) {
    String fn = "/" + String(now.tm_year + 1900 - 1) + ".dat";
    if (SPIFFS.exists(fn)) {
      fp = SPIFFS.open(fn, "r");
      if (fp) {
        while (fp.available()) {
          fp.read((uint8_t *)&kwh_days[kwh_days_p], sizeof(dataday));
          kwh_days_p = (kwh_days_p + 1) % KWH_DAYS;
        }
        fp.close();
      }
    }
    fn = "/" + String(now.tm_year + 1900) + ".dat";
    if (SPIFFS.exists(fn)) {
      fp = SPIFFS.open(fn, "r");
      if (fp) {
        while (fp.available()) {
          fp.read((uint8_t *)&kwh_days[kwh_days_p], sizeof(dataday));
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
  Serial.println(isotime(now));
  Serial.printf(PSTR("空闲ram:%ld\r\n"), ESP.getFreeHeap());
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
    SPIFFS.end();
  }
  loop_clock(false);
}
void day() {
  kwh_days[kwh_days_p].kwh = get_kwh() - nvram.kwh_day0;
  kwh_days[kwh_days_p].time = mktime(&now);
  nvram.kwh_day0 = get_kwh();
  if (now.tm_year > 2021 - 1900) {
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
void smart_config() {
  uint32_t colors[3] = {0xf00000, 0x00f000, 0x0000f0};
  //手机连上2.4G的wifi,然后微信打开网页：http://wx.ai-thinker.com/api/old/wifi/config
  save_nvram();
  smart_status = 1;
  // if (wifi_connected_is_ok()) return true;
  WiFi.mode(WIFI_STA); //开AP
  WiFi.beginSmartConfig();
  for (uint16_t i = 0; i < 500; i++) {
    delay(200);
    system_soft_wdt_feed (); //各loop里要根据需要执行喂狗命令
    led_send(colors[i % 3]);
    yield();
    if (smart_status == 2 && digitalRead(KEYWORD) == LOW) { //松开按键后，又按下按键
      Serial.println(F("key down exit"));
      WiFi.stopSmartConfig();
      return;
    }
    if (smart_status == 1 && digitalRead(KEYWORD) == HIGH)
      smart_status = 2; //按键已经松开
    if (WiFi.smartConfigDone()) {
      wifi_set_clean();
      wifi_set_add(WiFi.SSID().c_str(), WiFi.psk().c_str());
      WiFi.setAutoConnect(true);
      Serial.println(F("OK"));
      WiFi.stopSmartConfig();
      return;
    }
    if (i % 5 == 0)
      Serial.write('.');
    if (i % 100 == 0)
      Serial.println();
    yield();
    system_soft_wdt_feed (); //各loop里要根据需要执行喂狗命令
    if (wifi_connected_is_ok()) {
      httpd_loop();
    }
  }
  WiFi.stopSmartConfig();
}
