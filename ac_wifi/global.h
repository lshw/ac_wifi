#ifndef __GLOBAL_H__
#define __GLOBAL_H__
#include "config.h"
#include "nvram.h"
#include <time.h>
#include "Ticker.h"
#include <ESP8266WiFiMulti.h>
#include <WiFiUdp.h>

#include "ws2813.h"
#include "pwm_speeker.h"
#include "datalog.h"
#include "CRC32.h"
CRC32 crc;
Ticker _myTicker;
extern float i_max;
int16_t update_timeok = 0;  //0-马上wget ，-1 关闭，>0  xx分钟后wget
uint8_t timer3 = 30;        //最长30秒等待上线
uint16_t i_over = 0;        //电流过高保护， 倒计时ms
uint32_t switch_change_time = 0;
bool wifi_connected_is_ok();
extern uint8_t sound_buf[100];
uint16_t http_get(uint8_t);
struct set0 {
  uint8_t relink : 1;
  uint8_t reboot_now : 1;
  uint8_t connected_is_ok : 1;
  uint8_t power_down : 1;
} set0;
uint16_t wget() {
  uint16_t httpCode = http_get(nvram.nvram7 & NVRAM7_URL);  //先试试上次成功的url
  if (httpCode < 200 || httpCode >= 400) {
    nvram.nvram7 = (nvram.nvram7 & ~NVRAM7_URL) | (~nvram.nvram7 & NVRAM7_URL);
    save_nvram();
    nvram_save = 0;                                  //不需要保存 url选择， 到file
    httpCode = http_get(nvram.nvram7 & NVRAM7_URL);  //再试试另一个的url
  }
  return httpCode;
}

String get_url(uint8_t no) {
  File fp;
  char fn[20];
  String ret;
  if (no == 0 || no == '0') ret = String(DEFAULT_URL0);
  else ret = String(DEFAULT_URL1);
  if (SPIFFS.begin()) {
    if (no == 0 || no == '0')
      fp = SPIFFS.open("/url.txt", "r");
    else
      fp = SPIFFS.open("/url1.txt", "r");
    if (fp) {
      ret = fp.readStringUntil('\n');
      ret.trim();
      fp.close();
      if (ret.startsWith("http://www.cfido.com/")) {
        SPIFFS.remove("/url.txt");
        SPIFFS.remove("/url1.txt");
        ret.replace("www.cfido.com/", "temp.cfido.com:808/");
      } else if (ret.startsWith("http://www.wf163.com/")) {
        SPIFFS.remove("/url.txt");
        SPIFFS.remove("/url1.txt");
        ret.replace("www.wf163.com/", "temp2.wf163.com:808/");
      }
    }
  }
  SPIFFS.end();
  if (ret == "") {
    if (no == 0 || no == '0')
      ret = DEFAULT_URL0;
    else
      ret = DEFAULT_URL1;
  }
  return ret;
}
String get_ssid() {
  File fp;
  String ssid;
  if (SPIFFS.begin()) {
    fp = SPIFFS.open("/ssid.txt", "r");
    if (fp) {
      ssid = fp.readString();
      fp.close();
    } else {
      fp = SPIFFS.open("/ssid.txt", "w");
      ssid = "test:cfido.com";
      fp.println(ssid);
      fp.close();
    }
  } else
    Serial.print(F("载入ssid设置:"));
  Serial.println(ssid);
  SPIFFS.end();
  return ssid;
}

String fp_gets(File fp) {
  int ch = 0;
  String ret = "";
  while (1) {
    ch = fp.read();
    if (ch == -1) return ret;
    if (ch != 0xd && ch != 0xa) break;
  }
  while (ch != -1 && ch != 0xd && ch != 0xa) {
    ret += (char)ch;
    ch = fp.read();
  }
  ret.trim();
  return ret;
}

#define __YEAR__ ((((__DATE__[7] - '0') * 10 + (__DATE__[8] - '0')) * 10 \
                   + (__DATE__[9] - '0')) \
                    * 10 \
                  + (__DATE__[10] - '0'))

#define __MONTH__ (__DATE__[2] == 'n'   ? (__DATE__[1] == 'a' ? 1 : 6) /*Jan:Jun*/ \
                   : __DATE__[2] == 'b' ? 2 \
                   : __DATE__[2] == 'r' ? (__DATE__[0] == 'M' ? 3 : 4) \
                   : __DATE__[2] == 'y' ? 5 \
                   : __DATE__[2] == 'l' ? 7 \
                   : __DATE__[2] == 'g' ? 8 \
                   : __DATE__[2] == 'p' ? 9 \
                   : __DATE__[2] == 't' ? 10 \
                   : __DATE__[2] == 'v' ? 11 \
                                        : 12)

#define __DAY__ ((__DATE__[4] == ' ' ? 0 : __DATE__[4] - '0') * 10 \
                 + (__DATE__[5] - '0'))

void wifi_set_clean() {
  if (SPIFFS.begin()) {
    SPIFFS.remove("/ssid.txt");
    SPIFFS.end();
  }
}
void wifi_set_add(const char *wps_ssid, const char *wps_password) {
  File fp;
  int8_t mh_offset;
  String wifi_sets, line;
  if (wps_ssid[0] == 0) return;
  if (SPIFFS.begin()) {
    fp = SPIFFS.open("/ssid.txt", "r");
    wifi_sets = String(wps_ssid) + ":" + String(wps_password) + "\r\n";
    if (fp) {
      while (fp.available()) {
        line = fp.readStringUntil('\n');
        line.trim();
        if (line == "")
          continue;
        if (line.length() > 110)
          line = line.substring(0, 110);
        mh_offset = line.indexOf(':');
        if (mh_offset < 2) continue;
        if (line.substring(0, mh_offset) == wps_ssid)
          continue;
        else
          wifi_sets += line + "\r\n";
      }
      fp.close();
    }
    fp = SPIFFS.open("/ssid.txt", "w");
    if (fp) {
      fp.print(wifi_sets);
      fp.close();
    }
    SPIFFS.end();
  }
}

void dump_hex(char *msg, uint16_t len) {
  for (uint16_t i = 0; i < len; i++) {
    if ((i % 0x10) == 0) Serial.printf(PSTR("\r\n[%04X]"), i);
    if ((i % 0x10) == 8) Serial.write(' ');
    Serial.printf(PSTR(" %02X"), msg[i]);
  }
  Serial.println();
}

uint32_t calculateCRC32(const uint8_t *data, size_t length) {
  return CRC32::calculate(data, length);
}

uint32_t led_half() {
  return ((sets.color / 4) & 0xff0000)
         | (((sets.color & 0xff00) / 4) & 0xff00)
         | ((sets.color & 0xff) / 4);
}

void switch_change(bool onff) {
  switch_change_time = 0;
  digitalWrite(SSR, onff);
  if (onff == HIGH) {  //关机
    play((char *)"321");
    led_send(led_half());
  } else {  //开机
    play((char *)"123");
    led_send(sets.color);
  }
}
inline char *strncpy(char *dest, uint16_t size, const __FlashStringHelper *ifsh) {
  uint16_t i = 0;
  char bc;
  PGM_P p = reinterpret_cast<PGM_P>(ifsh);
  while (bc = pgm_read_byte(p + i)) {
    if (i >= size) break;
    dest[i] = bc;
    i++;
    dest[i] = 0;
  }
  return dest;
}
String int2str(uint8_t dat) {
  if (dat < 10) return String("0") + String(dat);
  return String(dat);
}
String time_ymd(struct tm tm0) {
  return String(tm0.tm_year + 1900)
         + "-" + int2str(tm0.tm_mon + 1)
         + "-" + int2str(tm0.tm_mday);
}
String time_ymd(time_t t) {
  struct tm tm0;
  gmtime_r(&t, &tm0);
  return time_ymd(tm0);
}

String isotime(struct tm tm0) {
  return time_ymd(tm0)
         + " " + int2str(tm0.tm_hour)
         + ":" + int2str(tm0.tm_min)
         + ":" + int2str(tm0.tm_sec);
}

#endif
