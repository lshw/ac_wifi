#ifndef __PWM_SPEEKER_H__
#define __PWM_SPEEKET_H__

void set_hz(uint8_t range) {
  uint16_t snd_high[7] = {1047, 1175, 1319, 1397, 1568, 1760, 1976};
  uint16_t snd[7] = {532, 587, 659, 698, 784, 880, 988};
  uint16_t snd_low[7] = {262, 294, 330, 349, 392, 440, 494};
  switch (range) {
    case 1 ... 9:
      analogWriteFreq(snd[range - 1]);
      break;
    case '1' ... '9':
      analogWriteFreq(snd[range - '1']);
      break;
    case 'a' ... 'g':
      analogWriteFreq(snd_low[range - 'a']);
      break;
    case 'A' ... 'G':
      analogWriteFreq(snd_high[range - 'A']);
      break;
  }
}

uint32_t sound_delay;
void sound(uint8_t range, uint16_t len) {
  set_hz(range);
  sound_delay = millis() + len;
}

void sound(uint8_t range) {
  sound(range, 500);
  while (sound_delay > millis()) {
    yield();
    system_soft_wdt_feed (); //喂狗命令
  }
}

uint8_t sound_buf[100];
void play(char * qz) {
  strncpy((char *)sound_buf, qz, sizeof(sound_buf));
  sound_buf[sizeof(sound_buf) - 1] = 0;
}
bool pwm_on = false;
void sound_20ms() {
  if (sound_delay > millis()) return;
  if (sound_buf[0] != 0) {
    if (!pwm_on) {
      analogWrite(5, 2);
      pwm_on = true;
    }
    sound(sound_buf[0], 500);
    Serial.printf("sound(%c)\r\n", sound_buf[0]);
    strncpy((char *)sound_buf, (char *)&sound_buf[1], sizeof(sound_buf) - 1);
  } else if (pwm_on) {
    analogWrite(5, 0);
    pwm_on = false;
  }
}


#endif  //__PWM_SPEEKER_H__
