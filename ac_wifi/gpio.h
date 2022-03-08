#ifndef __GPIO_H__
#define __GPIO_H__

#define SSD 4 //SSD
#define KEYWORD 14

uint32_t keydown_ms = 0;
ICACHE_RAM_ATTR void keydown() {
  if (keydown_ms + 20 > millis()) return;
  keydown_ms = millis();
  if (digitalRead(SSD) == HIGH) {
    Serial.println("down");
    digitalWrite(SSD, LOW);
    play("2");
  } else {
    Serial.println("up");
    digitalWrite(SSD, HIGH);
    play("1");
  }
}
void gpio_setup() {
  pinMode(SSD, OUTPUT);
  pinMode(0, INPUT);
  pinMode(KEYWORD, INPUT);
  digitalWrite(SSD, HIGH);
  //  attachInterrupt(digitalPinToInterrupt(D3), keydown, FALLING);
}
#endif //__GPIO_H__
