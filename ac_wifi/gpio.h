#ifndef __GPIO_H__
#define __GPIO_H__

uint32_t last_check_connected;
bool last_keygen = HIGH ;
uint32_t keydown_ms = 0;
void key_check() {
  if (last_keygen != digitalRead(KEYWORD)) {
    last_keygen = digitalRead(KEYWORD);
    if (last_keygen == LOW) {
      if (keydown_ms + 20 > millis()) return;
      keydown_ms = millis();
      if (digitalRead(SSR) == HIGH) {
        digitalWrite(SSR, LOW);
        play("321");
      } else {
        digitalWrite(SSR, HIGH);
        play("123");
      }
    }
  }
}

void gpio_setup() {
  pinMode(SSR, OUTPUT);
  digitalWrite(SSR, HIGH);
  pinMode(KEYWORD, INPUT_PULLUP);
}
#endif //__GPIO_H__
