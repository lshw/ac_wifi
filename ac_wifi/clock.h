#ifndef __CLOCK_H__
#define __CLOCK_H__
#include <time.h>
#include "global.h"
struct tm now;
volatile uint8_t time_update = 0;

#define MIN_UP 1
#define HOUR_UP 2
#define DAY_UP 4
#define SEC10_UP 8
inline void time_update_set(uint8_t flags) {
  noInterrupts();
  time_update |= flags;
  interrupts();  // codex修改: 节拍回调和主循环都会改写事件位，置位需放进临界区避免丢标志
}
inline uint8_t time_update_take() {
  uint8_t flags;
  noInterrupts();
  flags = time_update;
  time_update = 0;
  interrupts();  // codex修改: 主循环一次性取走并清空事件位，避免与回调并发时丢失分钟/小时事件
  return flags;
}
void wifi_status();
void sec10() {  //由loop调用
  wifi_status();
}
void sec() {
  now.tm_sec++;
  if ((now.tm_sec % 10) == 0)  // codex修改: 修正运算符优先级，按 10 秒周期触发
    time_update_set(SEC10_UP);  //10秒标志， 让loop去调用sec10();
  if (datamins[now.tm_min] < power)
    datamins[now.tm_min] = power;
  if (now.tm_sec >= 60) {
    now.tm_sec -= 60;
    now.tm_min++;
    time_update_set(MIN_UP);
    if (now.tm_min >= 60) {
      now.tm_min -= 60;
      now.tm_hour++;
      time_update_set(HOUR_UP);
      if (now.tm_hour >= 24) {
        now.tm_hour -= 24;
        now.tm_mday++;
        time_update_set(DAY_UP);
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
