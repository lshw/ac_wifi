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
#include "pwm_speeker.h"
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
  uint32_t now_ms = millis();
  while ((int32_t)(now_ms - dida0) >= 0) {
    dida0 += 1000;
    sec();  // codex修改: 用到期时间循环补跑秒节拍，避免启动初期和回调抖动时漏秒或秒计数漂移
  }
  count_100ms++;
  if (count_100ms >= 5) {
    count_100ms = 0;
    data100ms[data100ms_p] = power;
    data100ms_p = (data100ms_p + 1) % 600;
  }
}
void setup() {
  ESP.wdtEnable(50000);
  Serial.begin(4800, SERIAL_8E1);  //hlw8032需要这个速度
  load_set();                      //从files载入数据
  gpio_setup();
  load_nvram();  //从esp8266的nvram载入数据
  memset(&now, 0, sizeof(now));
  dida0 = millis() + 1000;  // codex修改: 秒节拍基于当前时间启动，避免从 0 起步导致早期快速补秒
  _myTicker.attach_ms(20, run_20ms);

  wifi_country_t mycountry = {
    .cc = "CN",
    .schan = 1,
    .nchan = 13,
    .policy = WIFI_COUNTRY_POLICY_MANUAL,
  };

  wifi_set_country(&mycountry);
  wifi_station_connect();
  pinMode(LEDP, OUTPUT);
  play((char *)"1");  //滴～～
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
  body.reserve(16384);  // codex修改: 首页包含多段图表数据，预留更大缓冲区以减少动态扩容和堆碎片
  Serial.printf(PSTR("空闲ram:%ld\r\n"), ESP.getFreeHeap());
}

uint32_t last_wget = 0;
uint32_t last_10sec = 0;
volatile uint8_t smart_status = 0;  // codex修改: 被 GPIO 中断读取、被主循环修改的共享状态需声明为 volatile
void loop() {
  struct tm now0;
  now_snapshot(&now0);
  if (key_toggle_pending) {
    noInterrupts();
    bool do_toggle = key_toggle_pending;
    key_toggle_pending = false;
    interrupts();
    if (do_toggle) {
      switch_change(!digitalRead(SSR));  // codex修改: 把重操作移到主循环执行
    }
  }
  uint8_t deferred_actions = deferred_action_take();
  if (deferred_actions & DEFER_SAVE_SWITCH) {
    sets.on_off = digitalRead(SSR);
    save_set(false);
    play((char *)"c");  // codex修改: 把定时回调里的保存和提示音移到主循环执行，避免在 Ticker 上下文访问 SPIFFS
  }
  if (deferred_actions & DEFER_SWITCH_OFF) {
    switch_change(LOW);  // codex修改: 自动开关动作改由主循环执行，避免回调里直接驱动继电器和灯带
  } else if (deferred_actions & DEFER_SWITCH_ON) {
    switch_change(HIGH);  // codex修改: 同一轮只执行一个自动切换方向，避免边界条件下连续翻转
  }
  if (deferred_actions & DEFER_WIFI_OFF) {
    wifi_off();  // codex修改: 把计量回调里的断网动作移到主循环执行，避免在 20ms 路径直接切换 WiFi 硬件
  }
  if (set0.relink) {
    set0.relink = false;
    wifi_setup();
    set0.connected_is_ok = false;
  }
  if (wifi_connected_is_ok()) {
    if (!set0.httpd_up) {
      play((char *)"23");
      set0.httpd_up = true;
      httpd_listen();
    }
    if (now0.tm_year < __YEAR__ - 1900 && set0.connected_is_ok) {
      Serial.println("getLocalTime()");
      Serial.println(getLocalTime(&now, 1000));
    }
    httpd_loop();
    uint32_t now_ms = millis();
    if (millis_reached(last_wget, now_ms)) {
      last_wget = now_ms + 1000 * 3600 * 4;  // codex修改: 用溢出安全的到期判断维持 4 小时上报节拍，避免长期运行后停止或提前触发
      wget();
    }
    yield();
    if (WiFi.status() != WL_CONNECTED) {
      set0.relink = true;
    }
  }
  system_soft_wdt_feed();
  uint8_t set_modi_flags = set_modi_read();
  if (set_modi_flags & SET_CHARGE) {
    save_set(false);  // 保存 /sets.txt
  }
  yield();
  uint8_t time_flags = time_update_take();
  if (time_flags & DAY_UP) {
    day();
    yield();
  }
  if (time_flags & HOUR_UP) {
    hour();
    yield();
  }
  if (time_flags & MIN_UP) {
    minute();
    yield();
  }
  if (time_flags & SEC10_UP) {
    sec10();
    yield();
  }
  system_soft_wdt_feed();
  if (set0.reboot_now) {
    Serial.println(F("reboot..."));
    Serial.flush();
    nvram_save_set(millis());
    save_nvram_file();
    set0.reboot_now = false;
    ESP.restart();
  }
  now_snapshot(&now0);
  if (kwh_days_p == -1 && now0.tm_year > 121) {
    load_kwh_days();
  }
  noInterrupts();
  uint32_t keydown_ms0 = keydown_ms;
  interrupts();
  if (smart_status == 0 && keydown_ms0 > 0 && millis() - keydown_ms0 > 5000 && digitalRead(KEYWORD) == LOW) {
    noInterrupts();
    keydown_ms = 0;  // codex修改: 先复制再判断，避免和按键中断并发读写
    interrupts();
    Serial.println(F("smart_config() begin"));
    smart_status = 1;
    smart_config();
    led_send(sets.color);
    smart_status = 3;  //退出进行中
    Serial.println(F("smart_config() end"));
  }
  if (smart_status == 3 && digitalRead(KEYWORD)) {  //等待松开按键就结束过程
    Serial.println(F("smart_config 结束"));
    smart_status = 0;
    wifi_off();
    set0.relink = true;
  }
#ifdef NETLOG
  netlog_loop();
#endif
}

void load_kwh_days() {
  File fp;
  struct tm now0;
  now_snapshot(&now0);
  kwh_days_p = 0;
  memset(kwh_days, 0, sizeof(kwh_days));
  if (SPIFFS.begin()) {
    String fn = year_dat_path(now0.tm_year + 1900 - 1);
    if (SPIFFS.exists(fn)) {
      fp = SPIFFS.open(fn, "r");
      if (fp) {
        while (fp.available() >= (int)sizeof(dataday)) {
          if (fp.read((uint8_t *)&kwh_days[kwh_days_p], sizeof(dataday)) != sizeof(dataday)) break;  // codex修改: 只接受完整记录
          kwh_days_p = (kwh_days_p + 1) % KWH_DAYS;
        }
        fp.close();
      }
    }
    fn = year_dat_path(now0.tm_year + 1900);
    if (SPIFFS.exists(fn)) {
      fp = SPIFFS.open(fn, "r");
      if (fp) {
        while (fp.available() >= (int)sizeof(dataday)) {
          if (fp.read((uint8_t *)&kwh_days[kwh_days_p], sizeof(dataday)) != sizeof(dataday)) break;  // codex修改: 只接受完整记录
          kwh_days_p = (kwh_days_p + 1) % KWH_DAYS;
        }
        fp.close();
      }
    }
    fn = "";
    SPIFFS.end();
  }
}
String year_dat_path(int year) {
  char path[16];
  snprintf(path, sizeof(path), "/%d.dat", year);
  return String(path);
}
extern float datamins[60];  //240 byte 每分钟最大功率
void minute() {
  struct tm now0;
  uint32_t nvram_save0 = nvram_save_read();
  uint32_t last_save0 = last_save_read();
  uint32_t now_ms = millis();
  now_snapshot(&now0);
  datamins[now0.tm_min] = 0.0;
  if ((now0.tm_min % 10) == 0)
    save_nvram();
  if ((nvram_save0 > 0 && millis_reached(nvram_save0, now_ms))
      || millis_reached(last_save0 + 120000, now_ms)
      || millis_before(last_save0, now_ms))
    save_nvram_file();
  Serial.println(isotime(now0));
  Serial.printf(PSTR("空闲ram:%ld\r\n"), ESP.getFreeHeap());
}
extern float datahour[24];  //96字节  每一小时的耗电量
void hour() {
  struct tm now0;
  now_snapshot(&now0);
  datahour[now0.tm_hour] = get_kwh() - nvram.kwh_hour0;
  nvram.kwh_hour0 = get_kwh();
  save_nvram();
  if (SPIFFS.begin()) {
    File fp;
    fp = SPIFFS.open("/hours.dat", "a");
    if (fp) {
      if (fp.write((uint8_t *)&datahour, sizeof(datahour)) != sizeof(datahour)) {
        Serial.println(F("hours.dat写入不完整"));  // codex修改: 统计文件写失败时输出明确日志，避免静默损坏
      }
      fp.close();
    } else {
      Serial.println(F("hours.dat打开失败"));  // codex修改: 补齐小时统计文件句柄检查，避免空句柄写入
    }
    SPIFFS.end();
  }
}
void day() {
  struct tm now0;
  now_snapshot(&now0);
  kwh_days[kwh_days_p].kwh = get_kwh() - nvram.kwh_day0;
  kwh_days[kwh_days_p].time = mktime(&now0);
  nvram.kwh_day0 = get_kwh();
  if (now0.tm_year > 2021 - 1900) {
    if (SPIFFS.begin()) {
      File fp;
      fp = SPIFFS.open(year_dat_path(now0.tm_year + 1900), "a");
      if (fp) {
        if (fp.write((uint8_t *)&kwh_days[kwh_days_p], sizeof(dataday)) != sizeof(dataday)) {
          Serial.println(F("日统计写入不完整"));  // codex修改: 年统计写失败时保留日志，避免文件损坏被静默吞掉
        }
        fp.close();
      } else {
        Serial.println(F("日统计文件打开失败"));  // codex修改: 补齐年统计文件句柄检查，避免空句柄写入
      }
      SPIFFS.end();
    }
    kwh_days_p = (kwh_days_p + 1) % KWH_DAYS;
  }
}
void smart_config() {
  uint32_t colors[3] = { 0xf00000, 0x00f000, 0x0000f0 };
  //手机连上2.4G的wifi,然后微信打开网页：http://wx.ai-thinker.com/api/old/wifi/config
  save_nvram();
  smart_status = 1;
  // if (wifi_connected_is_ok()) return true;
  WiFi.mode(WIFI_STA);  //开AP
  WiFi.beginSmartConfig();
  for (uint16_t i = 0; i < 500; i++) {
    delay(200);
    system_soft_wdt_feed();  //各loop里要根据需要执行喂狗命令
    led_send(colors[i % 3]);
    yield();
    if (smart_status == 2 && digitalRead(KEYWORD) == LOW) {  //松开按键后，又按下按键
      Serial.println(F("key down exit"));
      WiFi.stopSmartConfig();
      wifi_off();
      set0.relink = true;
      return;
    }
    if (smart_status == 1 && digitalRead(KEYWORD) == HIGH)
      smart_status = 2;  //按键已经松开
    if (WiFi.smartConfigDone()) {
      wifi_set_clean();
      wifi_set_add(WiFi.SSID().c_str(), WiFi.psk().c_str());
      WiFi.setAutoConnect(true);
      Serial.println(F("OK"));
      WiFi.stopSmartConfig();
      wifi_off();
      set0.relink = true;
      return;
    }
    if (i % 5 == 0)
      Serial.write('.');
    if (i % 100 == 0)
      Serial.println();
    yield();
    system_soft_wdt_feed();  //各loop里要根据需要执行喂狗命令
    if (wifi_connected_is_ok()) {
      httpd_loop();
    }
  }
  Serial.println(F("smart_config timeout"));  // codex修改: 超时退出时保留明确日志，避免和手动退出、成功退出混淆
  WiFi.stopSmartConfig();
  wifi_off();          // codex修改: 超时路径也统一关闭 WiFi，避免残留 smart config 状态影响后续重连
  set0.relink = true;  // codex修改: 超时退出后也走统一重连流程，保持与成功/手动退出一致
}
