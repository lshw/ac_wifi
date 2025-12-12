#ifndef __CLOCK_H__
#define __CLOCK_H__
#include <time.h>
#include "global.h"
struct tm now;
uint8_t time_update = 0;

#define MIN_UP 1
#define HOUR_UP 2
#define DAY_UP 4
#define SEC10_UP 8
void wifi_status();
void sec10() {  //由loop调用
  wifi_status();
}
void sec() {
  now.tm_sec++;
  if (now.tm_sec & 10 == 1)
    time_update |= SEC10_UP;  //10秒标志， 让loop去调用sec10();
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

#endif  //__CLOCK_H__
