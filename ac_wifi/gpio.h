#ifndef __GPIO_H__
#define __GPIO_H__

#define SSR 4 //SSD
#define KEYWORD 0
uint32_t last_check_connected;
bool last_keygen = HIGH ;
uint32_t keydown_ms = 0;
uint8_t ssr_change = 0;
void key_check() {
  if (last_keygen != digitalRead(KEYWORD)) {
    last_keygen = digitalRead(KEYWORD);
    if (last_keygen == LOW) {
      if (keydown_ms + 20 > millis()) return;
      keydown_ms = millis();
      ssr_change |= 0x80;
      if (digitalRead(SSR) == HIGH) {
        ssr_change &= ~1;
        digitalWrite(SSR, LOW);
      } else {
        ssr_change |= 1;
        digitalWrite(SSR, HIGH);
      }
    }
  }
  get_ac();
}

void gpio_setup() {
  pinMode(SSR, OUTPUT);
  digitalWrite(SSR, HIGH);
  pinMode(KEYWORD, INPUT_PULLUP);
  _myTicker.attach_ms(20, key_check);
}
#endif //__GPIO_H__
