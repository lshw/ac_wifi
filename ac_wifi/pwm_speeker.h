#ifndef __PWM_SPEEKER_H__
#define __PWM_SPEEKET_H__

void sound(uint8_t range, uint16_t len) {
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
  delay(len);
}

void sound(uint8_t range) {
  sound(range, 500);
}

void play(char * qz) {
  uint16_t i0 = strlen(qz);
  for (uint16_t i = 0; i < i0; i++) sound(qz[i] & 0xf);
}

#endif  //__PWM_SPEEKER_H__
