#ifndef __NVRAM__
#define __NVRAM__

#define NVRAM7_URL    0b10
#define NVRAM7_UPDATE 0b1000

#define NVRAM_CHARGE 0b1
#define SET_FILE_CHARGE 0b10

uint8_t set_modi = 0; //不等于0, 需要保存
uint8_t set_file_modi = 0; //不等于0, 需要保存
void fix_ac_set();
struct {
  uint8_t nc;
  uint8_t nvram7;
  uint8_t change;
  uint8_t ch;
  uint32_t boot_count;
  double  kwh; //总度数
  uint32_t  ac_pf; //未换算成度数的pf计数,  超过 sets.ac_kwh_count  就进1到ac_kwh
  uint32_t  ac_pf0; //已统计的HLW8032的pf 如果实际的小于这个数据， 就要把它加上8032的读数， 加到 sets.ac_pf, 并且设置本行为8032读数。
  uint32_t crc32;
} __attribute__ ((packed)) nvram;

struct { //不会经常变化的设置， 需要保存到文件系统 sets.dat
  uint16_t ac_power_change_value;
  uint16_t ac_voltage_change_value;
  uint16_t ac_alert_minute;
  float ac_v_calibration;
  float ac_i_calibration;
} __attribute__ ((packed)) sets; //字节紧凑格式， 不做字对齐

uint32_t calculateCRC32(const uint8_t *data, size_t length);

void load_nvram() {
  File fp;
  ESP.rtcUserMemoryRead(0, (uint32_t*) &nvram, sizeof(nvram));
  if (nvram.crc32 != calculateCRC32((uint8_t*) &nvram, sizeof(nvram) - sizeof(nvram.crc32))) {
    SPIFFS.begin();
    if (SPIFFS.exists("/nvram.txt")) {
      fp = SPIFFS.open("/nvram.txt", "r");
      fp.read((uint8_t *)&nvram, sizeof(nvram));
      fp.close();
    }
    SPIFFS.end();
    if (nvram.crc32 != calculateCRC32((uint8_t*) &nvram, sizeof(nvram) - sizeof(nvram.crc32))) {
      memset(&nvram, 0, sizeof(nvram));
      fix_ac_set();
    }
  } else {
    Serial.println("\r\nwifi channel=" + String(nvram.ch) );
    WRITE_PERI_REG(0x600011f4, 1 << 16 | nvram.ch);
  }
}

void save_nvram() {
  if (nvram.change == 0) return;
  nvram.change = 0;
  nvram.crc32 = calculateCRC32((uint8_t*) &nvram, sizeof(nvram) - sizeof(nvram.crc32));
  ESP.rtcUserMemoryWrite(0, (uint32_t*) &nvram, sizeof(nvram));
}

void save_nvram_file() {
  File fp;
  SPIFFS.begin();
  fp = SPIFFS.open("/nvram.txt", "w");
  fp.write((uint8_t *)&nvram, sizeof(nvram));
  fp.close();
  SPIFFS.end();
}

uint32_t calculateCRC32(const uint8_t *data, size_t length) {
  uint32_t crc = 0xffffffff;
  while (length--) {
    uint8_t c = *data++;
    for (uint32_t i = 0x80; i > 0; i >>= 1) {
      bool bit = crc & 0x80000000;
      if (c & i) {
        bit = !bit;
      }
      crc <<= 1;
      if (bit) {
        crc ^= 0x04c11db7;
      }
    }
  }
  return crc;
}
#endif //__NVRAM__
