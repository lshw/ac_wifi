#ifndef __WIFI_CLIENT_H__
#define __WIFI_CLIENT_H__
#include "config.h"
#include <ESP8266WiFi.h>
#include <ESP8266HTTPClient.h>
#include <ESP8266WiFiMulti.h>
#include <ESP8266httpUpdate.h>
bool http_update();
ESP8266WiFiMulti WiFiMulti;
WiFiClient client;
HTTPClient http;
String ssid, passwd;
bool http_response_is_update(HTTPClient &http) {
  WiFiClient *stream = http.getStreamPtr();
  char payload[16];
  size_t len = 0;
  unsigned long deadline = millis() + 1000;
  while (stream && stream->connected() && millis() < deadline && len < sizeof(payload) - 1) {
    while (stream->available() && len < sizeof(payload) - 1) {
      char ch = stream->read();
      if (ch == '\r' || ch == '\n' || ch == ' ' || ch == '\t') continue;
      payload[len++] = ch;
      if (len >= 6) break;  // codex修改: 只保留 UPDATE 指令所需的最小响应，避免把整页响应读入 String
    }
    if (len >= 6) break;
    delay(1);
    yield();
  }
  payload[len] = 0;
  for (size_t i = 0; i < len; i++) {
    if (payload[i] >= 'a' && payload[i] <= 'z') payload[i] -= 32;
  }
  return strcmp(payload, "UPDATE") == 0;
}
uint8_t hex2ch(char dat) {
  dat |= 0x20;  //41->61 A->a
  if (dat >= 'a') return dat - 'a' + 10;
  return dat - '0';
}
void hexprint(uint8_t dat) {
  if (dat < 0x10) Serial.write('0');
  Serial.print(dat, HEX);
}

void wifi_off() {
  WiFi.disconnect(true);  // 断开并关闭WiFi硬件
  WiFi.mode(WIFI_OFF);
  set0.connected_is_ok = false;
}
void wifi_setup() {
  File fp;
  uint32_t i;
  char buf[3];
  char ch;
  uint8_t count = 0;
  boolean is_ssid = true;
  WiFi.mode(WIFI_STA);
  set0.connected_is_ok = false;
  WiFi.hostname(hostname);
  WiFiMulti.cleanAPlist();  // codex修改: 重连前先清空旧列表，避免重复 addAP 导致内存持续增长
  //wifi_set_sleep_type(LIGHT_SLEEP_T);
  WiFi.setAutoConnect(true);    //自动链接上次
  WiFi.setAutoReconnect(true);  //断线自动重连
  if (SPIFFS.begin()) {
    if (!SPIFFS.exists("/ssid.txt")) {
      fp = SPIFFS.open("/ssid.txt", "w");
      fp.println(F("test:cfido.com"));
      fp.close();
    }
    fp = SPIFFS.open("/ssid.txt", "r");
    Serial.print(F("载入wifi设置文件:/ssid.txt"));
    ssid = "";
    passwd = "";
    if (fp) {
      uint16_t Fsize = fp.size();
      Serial.print(Fsize);
      Serial.println(F("字节"));
      for (i = 0; i < Fsize; i++) {
        ch = fp.read();
        switch (ch) {
          case 0xd:
          case 0xa:
            if (ssid != "") {
              if (count < 255) count++;  // codex修改: 正常换行结尾的配置也要计数，避免误加默认 AP
              Serial.print(F("Ssid:"));
              Serial.println(ssid);
              Serial.print(F("Passwd:"));
              Serial.println(passwd);
              WiFiMulti.addAP(ssid.c_str(), passwd.c_str());
            }
            is_ssid = true;
            ssid = "";
            passwd = "";
            break;
          case ' ':
          case ':':
            is_ssid = false;
            break;
          default:
            if (is_ssid)
              ssid += ch;
            else
              passwd += ch;
        }
      }
      if (ssid != "") {
        if (count < 5) count++;
        Serial.print(F("Ssid:"));
        Serial.println(ssid);
        Serial.print(F("Passwd:"));
        Serial.println(passwd);
        WiFiMulti.addAP(ssid.c_str(), passwd.c_str());
      }
    }
    if (count == 0)
      WiFiMulti.addAP("test", "cfido.com");
    if (fp) fp.close();
    SPIFFS.end();
  }
  WiFiMulti.run(5000);
  wifi_connected_is_ok();
}
void wifi_status() {
  wl_status_t status = WiFi.status();
  switch (status) {
    case WL_CONNECTED:
      Serial.println("WiFi 已连接");
      break;
    case WL_NO_SSID_AVAIL:
      Serial.println("找不到指定WiFi");
      break;
    case WL_CONNECT_FAILED:
      Serial.println("WiFi连接失败");
      break;
    case WL_IDLE_STATUS:
      Serial.println("WiFi空闲");
      break;
    case WL_DISCONNECTED:
      Serial.println("WiFi断开");
      break;
    default:
      Serial.println("未知状态");
  }
}
bool wifi_connected_is_ok() {
  if (set0.connected_is_ok)
    return set0.connected_is_ok;
  if (wifi_station_get_connect_status() == STATION_GOT_IP) {
    Serial.println("ip:" + WiFi.localIP().toString());
    set0.connected_is_ok = true;
    //  ht16c21_cmd(0x88, 0); //停止闪烁
    if (nvram.ch != wifi_get_channel()) {
      nvram.ch = wifi_get_channel();
      save_nvram();
    }
    if (strlen(sets.ntp) < 4) {
      configTime(sets.tz * 60, 0, "ntp.tuna.tsinghua.edu.cn", "ntp.tencent.com", "ntp.aliyun.com");
    } else
      configTime(sets.tz * 60, 0, sets.ntp, "ntp.tuna.tsinghua.edu.cn", "ntp.aliyun.com");
    uint8_t ap_id = wifi_station_get_current_ap_id();
    struct station_config config[5];
    wifi_station_get_ap_info(config);
    config[ap_id].bssid_set = 1;              //同名ap，mac地址不同
    wifi_station_set_config(&config[ap_id]);  //保存成功的ssid,用于下次通讯
#ifdef NETLOG
    netlog_setup();
#endif
    return true;
  }
  return false;
}

uint16_t http_get(uint8_t no) {
  String url0 = get_url(no);
  String wifi_ssid = WiFi.SSID();
  String wifi_bssid = WiFi.BSSIDstr();
  char numbuf[32];
  url0.reserve(url0.length() + 192);  // codex修改: 周期上报路径预留空间，减少多参数拼接时的反复扩容
  if (url0.indexOf('?') > 0)
    url0 += '&';
  else
    url0 += '?';
  url0 += F("ver=" VER "&sn=");
  url0 += hostname;
  url0 += F("&kwh=");
  snprintf(numbuf, sizeof(numbuf), "%.8f", get_kwh());
  url0 += numbuf;
  url0 += F("&v=");
  snprintf(numbuf, sizeof(numbuf), "%d", sets.vol);
  url0 += numbuf;
  url0 += F("&w=");
  snprintf(numbuf, sizeof(numbuf), "%.0f", power);
  url0 += numbuf;
  url0 += F("&i=");
  snprintf(numbuf, sizeof(numbuf), "%.3f", current);
  url0 += numbuf;
  url0 += F("&pf=");
  snprintf(numbuf, sizeof(numbuf), "%.2f", power_ys * 100.0);
  url0 += numbuf;
  url0 += F("&ssid=");
  url0 += wifi_ssid;
  url0 += F("&bssid=");
  url0 += wifi_bssid;
  url0 += F("&GIT=" GIT_VER "&rssi=");
  snprintf(numbuf, sizeof(numbuf), "%d", WiFi.RSSI());
  url0 += numbuf;
  url0 += F("&ms=");
  snprintf(numbuf, sizeof(numbuf), "%lu", millis());
  url0 += numbuf;
  Serial.println(url0);      //串口输出
  http.begin(client, url0);  //HTTP提交
  http.setTimeout(4000);
  int httpCode;
  for (uint8_t i = 0; i < 3; i++) {
    httpCode = http.GET();
    if (httpCode < 0) {
      Serial.write('E');
      delay(20);
      continue;
    }
    // httpCode will be negative on error
    if (httpCode >= 200 && httpCode <= 299) {
      // HTTP header has been send and Server response header has been handled
      Serial.print(F("[HTTP] GET... code:"));
      Serial.println(httpCode);
      // file found at server
      if (httpCode == HTTP_CODE_OK) {
        if (http_response_is_update(http)) {
          if (http_update() == false)
            http_update();
        }
      }
      break;
    } else {
      Serial.print(F("http error code "));
      Serial.println(httpCode);
      break;
    }
  }
  http.end();  // codex修改: 释放 HTTP 连接，避免长时间运行后堆碎片和句柄泄漏
  url0 = "";
  return httpCode;
}

void update_progress(int cur, int total) {
  Serial.printf(PSTR("HTTP update process at %d of %d bytes...\r\n"), cur, total);
}

bool http_update() {
  String update_url = "http://ac_wifi.anheng.com.cn/firmware.php?type=AC_WIFI&SN=" + hostname + "&GIT=" GIT_VER "&ver=" VER;  //可以在header里下发x-MD5作为校验
  Serial.print(F("下载firmware from "));
  Serial.println(update_url);
  ESPhttpUpdate.onProgress(update_progress);
  t_httpUpdate_return ret = ESPhttpUpdate.update(client, update_url);
  update_url = "";

  switch (ret) {
    case HTTP_UPDATE_FAILED:
      Serial.printf(PSTR("HTTP_UPDATE_FAILD Error (%d): %s\r\n"), ESPhttpUpdate.getLastError(), ESPhttpUpdate.getLastErrorString().c_str());
      return false;
      break;

    case HTTP_UPDATE_NO_UPDATES:
      Serial.println(F("HTTP_UPDATE_NO_UPDATES"));
      break;

    case HTTP_UPDATE_OK:
      Serial.println(F("HTTP_UPDATE_OK"));
      return true;
      break;
  }
  delay(1000);
  return false;
}
#endif __WIFI_CLIENT_H__
