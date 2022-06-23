#ifndef __GPIO_H__
#define __GPIO_H__

uint32_t last_check_connected;
bool last_keygen = HIGH ;
uint32_t keydown_ms = 0;
void key_check() {//20ms检查一次
  if (last_keygen != digitalRead(KEYWORD)) { //按键状态有变化
    last_keygen = digitalRead(KEYWORD);
    if (last_keygen == HIGH) { //松开按键
      if (keydown_ms + 20 > millis()) return; //按下短于20ms 算抖动
      if (keydown_ms == 0) return;
      keydown_ms = 0;
      if (millis() - keydown_ms > 5000) {
        return; //按下超过 10秒， 是进入smartconf状态;
      }
      if (digitalRead(SSR) == LOW) {
        digitalWrite(SSR, HIGH);
        play("321");
      } else {
        digitalWrite(SSR, LOW);
        play("123");
      }
    } else {//按下按键
      keydown_ms = millis(); //开始计时
    }
  }
}

void gpio_setup() {
  pinMode(SSR, OUTPUT);
  digitalWrite(SSR, LOW);
  pinMode(KEYWORD, INPUT_PULLUP);
}
#endif //__GPIO_H__
