#ifndef __GPIO_H__
#define __GPIO_H__
extern volatile uint8_t smart_status;
volatile uint32_t keydown_ms = 0;
volatile bool key_toggle_pending = false;
void ICACHE_RAM_ATTR key_int() {
  if (smart_status > 0) // 正在配网的话，关闭按键，
    return;
  bool key = digitalRead(KEYWORD);
  if (key == LOW) {        // 按下按键
    keydown_ms = millis(); // 开始计时
  } else {                 // 松开按键
    if (keydown_ms == 0)
      return; // 忽略
    if ((uint32_t)(millis() - keydown_ms) < 20)
      return; // codex修改: 用时间差做消抖，避免 millis 溢出后把短按误判成长按
    if (keydown_ms >= 0 && millis() - keydown_ms > 5000) {
      keydown_ms = 0;
      return; // 按下超过 10秒， 是进入smartconf状态;
    }
    key_toggle_pending =
        true; // codex修改: 中断里只置位，避免在 ISR 中执行蜂鸣器和 LED 逻辑
    keydown_ms = 0;
  }
}
void gpio_setup() {
  pinMode(SSR, OUTPUT);
  digitalWrite(SSR, sets.on_off);
  pinMode(KEYWORD, INPUT_PULLUP);
  attachInterrupt(digitalPinToInterrupt(KEYWORD), key_int, CHANGE);
}
#endif //__GPIO_H__
