#ifndef __CLOCK_H__
#define __CLOCK_H__
#include <time.h>
#include "global.h"
struct tm now;
uint8_t time_update = 0;

#define MIN_UP 1
#define HOUR_UP 2
#define DAY_UP 4
void sec() {
  now.tm_sec++;
  if (datamins[now.tm_min] < power)
    datamins[now.tm_min] = power;
  if (now.tm_sec >= 60) {
    now.tm_sec -= 60;
    now.tm_min++;
    time_update |= MIN_UP;
    if (now.tm_min >= 60) {
      now.tm_min -= 60;
      now.tm_hour++;
      time_update |= HOUR_UP;
      if (now.tm_hour >= 24) {
        now.tm_hour -= 24;
        now.tm_mday++;
        time_update |= DAY_UP;
        mktime(&now);  //修正日期
      }
    }
  }
  if (switch_change_time > 60 && switch_change_time < 65 && sets.on_off != digitalRead(SSR)) {  //switch状态改变1分钟后， 保存
    sets.on_off = digitalRead(SSR);
    save_set(false);
    play((char *)"c");
  }
  switch_change_time++;
  if (digitalRead(SSR) == HIGH) {  //now off
    if (sets.switch_off_time > 0 && sets.switch_off_time < switch_change_time)
      switch_change(LOW);
  } else {  //now on
    if (sets.switch_on_time > 0 && sets.switch_on_time < switch_change_time)
      switch_change(HIGH);
  }
}

#include <Udp.h>
WiFiUDP ntpUDP;
#define FIX_1900 2208988800UL
#define FIX_2036 2085978495UL  //从2036-02-07 进入下一圈
#define NTP_PACKET_SIZE 48
bool ntp_get(const char *ServerName) {
  uint32_t ret;
  byte buff[NTP_PACKET_SIZE];
  Serial.println((char *)ServerName);
  ntpUDP.begin(123);
  ntpUDP.beginPacket(ServerName, 123);  //NTP requests are to port 123
  memset(buff, 0, sizeof(buff));
  buff[0] = 0b11100011;  // LI, Version, Mode
  buff[1] = 0;           // Stratum, or type of clock
  buff[2] = 6;           // Polling Interval
  buff[3] = 0xEC;        // Peer Clock Precision
  // 8 bytes of zero for Root Delay & Root Dispersion
  buff[12] = 'I';
  buff[13] = 'N';
  buff[14] = 'I';
  buff[15] = 'R';
  ntpUDP.write(buff, NTP_PACKET_SIZE);
  ntpUDP.endPacket();

  uint16_t timeout = 0;
  int cb = 0;
  do {
    cb = ntpUDP.parsePacket();
    if (timeout > 100) return false;  // timeout after 1000 ms
    timeout++;
    system_soft_wdt_feed();  //各loop里要根据需要执行喂狗命令
    yield();
    delay(10);
  } while (cb == 0);

  ntpUDP.read(buff, NTP_PACKET_SIZE);
  ret = (uint32_t)(buff[40] << 24) | (buff[41] << 16) | (buff[42] << 8) | buff[43];
  if (ret > FIX_1900)  //ntp时间格式到time_t的转换，
    ret -= FIX_1900;
  else
    ret + FIX_2036;
  time_t t = ret;  //必须放一行这个， 否则不正常， 估计是gcc过度优化造成的
  t += 3600 * sets.tz;
  gmtime_r(&t, &now);
  Serial.println(now.tm_hour);
  return true;
}

bool ntp_get(const __FlashStringHelper *ServerName) {
  char server[20];
  strncpy((char *)server, sizeof(server), ServerName);
  return ntp_get((char *)server);
}
uint8_t ntp_last = 0;
void loop_clock(bool always) {
  if (!always) {
    if ((millis() % 60) != 0) return;     //随机进行授时
    if (ntp_last == now.tm_mday) return;  //每天搞一次
  }
  if (wifi_connected_is_ok()) {
    if (strlen(sets.ntp) < 4 || !ntp_get((char *)sets.ntp))
      if (!ntp_get(F("cn.ntp.org.cn"))
          && !ntp_get(F("ntp.ntsc.ac.cn"))
          && !ntp_get(F("3.openwrt.pool.ntp.org")))
        ntp_get(F("ntp.anheng.com.cn"));
    ntp_last = now.tm_mday;
  }
}
#endif  //__CLOCK_H__
