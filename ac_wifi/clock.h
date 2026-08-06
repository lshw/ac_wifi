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
uint8_t wifi_fail = 0;
void sec10() {  //由loop调用
  wifi_status();
  if (WiFi.localIP() == IPAddress(0, 0, 0, 0)) {
    wifi_fail++;
    if(wifi_fail >= 10) {
    wifi_fail = 0;
    wifi_setup();
    }
  }else wifi_fail = 0;
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
    datamins[now.tm_min % 60] = 0.0;  // codex修改: 分钟切换时立即清空新分钟槽位，避免主循环稍后再清零把新分钟前几秒的峰值抹掉
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
    deferred_action_set(DEFER_SAVE_SWITCH);  // codex修改: 定时回调里只置延后动作，避免在 Ticker 上下文直接访问 SPIFFS 和蜂鸣器
  }
  switch_change_time++;
  if (digitalRead(SSR) == HIGH) {  //now off
    if (sets.switch_off_time > 0 && sets.switch_off_time < switch_change_time)
      deferred_action_set(DEFER_SWITCH_OFF);  // codex修改: 自动开关动作改到主循环执行，避免回调里直接操作继电器和灯带
  } else {  //now on
    if (sets.switch_on_time > 0 && sets.switch_on_time < switch_change_time)
      deferred_action_set(DEFER_SWITCH_ON);  // codex修改: 自动开关动作改到主循环执行，保持回调里只做状态推进
  }
}

#endif  //__CLOCK_H__
