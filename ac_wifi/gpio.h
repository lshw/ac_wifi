#ifndef __GPIO_H__
#define __GPIO_H__
extern uint8_t smart_status;
uint32_t keydown_ms = 0;
void ICACHE_RAM_ATTR key_int() {
  if (smart_status > 0) //正在配网的话，关闭按键，
    return;
  bool key = digitalRead(KEYWORD);
  if (key == LOW) { //按下按键
    keydown_ms = millis(); //开始计时
  } else {//松开按键
    if (keydown_ms == 0) return; //忽略
    if (keydown_ms + 20 > millis()) return; //按下时长短于20ms 算抖动
    if (keydown_ms >= 0 && millis() - keydown_ms > 5000) {
      keydown_ms = 0;
      return; //按下超过 10秒， 是进入smartconf状态;
    }
    if (digitalRead(SSR) == LOW) {
      digitalWrite(SSR, HIGH);
      play("321");
    } else {
      digitalWrite(SSR, LOW);
      play("123");
    }
  keydown_ms = 0;
  }
}
void gpio_setup() {
  pinMode(SSR, OUTPUT);
  digitalWrite(SSR, LOW);
  pinMode(KEYWORD, INPUT_PULLUP);
  attachInterrupt(digitalPinToInterrupt(KEYWORD), key_int, CHANGE);
}
#endif //__GPIO_H__
