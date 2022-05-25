#ifndef __CLOCK_H__
#define __CLOCK_H__
#include <time.h>
struct tm now;
uint8_t time_update = 0;

#define MIN_UP 1
#define HOUR_UP 2
#define DAY_UP 4
void dida() {
  now.tm_sec++;
  if (now.tm_sec >= 60) {
    now.tm_sec -= 60;
    now.tm_min ++;
    time_update |= MIN_UP;
    if (now.tm_min >= 60) {
      now.tm_min -= 60;
      now.tm_hour ++;
      time_update |= HOUR_UP;
      if (now.tm_hour >= 24) {
        now.tm_hour -= 24;
        now.tm_mday ++;
        time_update |= DAY_UP;
        mktime(&now); //修正日期
      }
    }
  }
}

#include <Udp.h>
WiFiUDP ntpUDP;
#define FIX_1900 2208988800UL
#define FIX_2036 2085978495UL //从2036-02-07 进入下一圈
#define NTP_PACKET_SIZE 48
bool ntp_get(const char * ServerName) {
  uint32_t ret;
  byte  buff[NTP_PACKET_SIZE];
  memset(buff, 0, sizeof(buff));
  buff[0] = 0b11100011;   // LI, Version, Mode
  buff[1] = 0;     // Stratum, or type of clock
  buff[2] = 6;     // Polling Interval
  buff[3] = 0xEC;  // Peer Clock Precision
  // 8 bytes of zero for Root Delay & Root Dispersion
  buff[12]  = 'I';
  buff[13]  = 'N';
  buff[14]  = 'I';
  buff[15]  = 'R';
  ntpUDP.begin(123);
  ntpUDP.beginPacket(ServerName, 123); //NTP requests are to port 123
  ntpUDP.write(buff, NTP_PACKET_SIZE);
  ntpUDP.endPacket();

  uint16_t timeout = 0;
  int cb = 0;
  do {
    cb = ntpUDP.parsePacket();
    if (timeout > 100) return false; // timeout after 1000 ms
    timeout++;
    system_soft_wdt_feed (); //各loop里要根据需要执行喂狗命令
    yield();
    delay ( 10 );
  } while (cb == 0);

  ntpUDP.read(buff, NTP_PACKET_SIZE);
  ret = (uint32_t) (buff[40] << 24) | (buff[41] << 16) | (buff[42] << 8) | buff[43];
  if (ret > FIX_1900) //ntp时间格式到time_t的转换，
    ret -= FIX_1900;
  else
    ret + FIX_2036;
  time_t t = ret; //必须放一行这个， 否则不正常， 估计是gcc过度优化造成的
  gmtime_r(&t, &now);
  return true;
}

uint8_t ntp_last = 0;
uint32_t ntp_last_ms = 0;
void loop_clock() {
  time_t t;
  if (ntp_last != now.tm_mday) {
    if (wifi_connected) {
      if (ntp_last_ms + 60000 < millis() && ntp_last_ms  == 0) {
        ntp_last_ms = millis();
        if (!ntp_get("ntp.anheng.com.cn"))
          ntp_get("1.debian.pool.ntp.org");
      }
    }
  }
}
#endif //__CLOCK_H__
