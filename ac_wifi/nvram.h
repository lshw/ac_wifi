#ifndef __NVRAM__
#define __NVRAM__

#define NVRAM7_URL    0b10
#define NVRAM7_UPDATE 0b1000

#define SET_CHARGE 0b1

void update_kwh_count(); //更新kwh的脉冲数，
uint8_t set_modi = 0; //不等于0, 需要保存
struct {
  uint8_t nc;
  uint8_t nvram7;
  uint8_t reserved;
  uint8_t ch;
  uint32_t boot_count;
  double  kwh; //总度数
  uint32_t  ac_pf; //未换算成度数的pf计数,  超过 sets.ac_kwh_count  就进1到ac_kwh
  uint32_t  ac_pf0; //已统计的HLW8032的pf 如果实际的小于这个数据， 就要把它加上8032的读数， 加到 sets.ac_pf, 并且设置本行为8032读数。
  uint32_t crc32;
} __attribute__ ((packed)) nvram;
uint32_t nvram_save = 0;
struct { //不会经常变化的设置， 需要保存到文件系统 sets.dat
  uint32_t reserved0;
  uint32_t reserved1;
  float ac_v_calibration;
  float ac_i_calibration;
  uint32_t crc32;
} __attribute__ ((packed)) sets; //字节紧凑格式， 不做字对齐

uint32_t calculateCRC32(const uint8_t *data, size_t length);

void save_nvram() {
  nvram.crc32 = calculateCRC32((uint8_t*) &nvram, sizeof(nvram) - sizeof(nvram.crc32));
  ESP.rtcUserMemoryWrite(0, (uint32_t*) &nvram, sizeof(nvram));
  nvram_save = millis() + 60000; //60秒后 保存 nvram到 file
}

void save_nvram_file() {
  File fp;
  if (nvram_save == 0) return;
  if (nvram_save > millis()) {
    if (nvram_save - millis() < 600000) //可能millis() 溢出
      return;
  }
  SPIFFS.begin();
  fp = SPIFFS.open("/nvram.txt", "w");
  fp.write((uint8_t *)&nvram, sizeof(nvram));
  fp.close();
  SPIFFS.end();
  nvram_save = 0;
}

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
      update_kwh_count(); //校准数据初始化
    }
    save_nvram();
  } else {
    Serial.println("\r\nwifi channel=" + String(nvram.ch) );
    WRITE_PERI_REG(0x600011f4, 1 << 16 | nvram.ch);
  }
}

void save_set() {
  File fp;
  sets.crc32 = calculateCRC32((uint8_t*) &sets, sizeof(sets) - sizeof(sets.crc32));
  SPIFFS.begin();
  fp = SPIFFS.open("/sets.txt", "w");
  fp.write((uint8_t *)&sets, sizeof(sets));
  fp.close();
  SPIFFS.end();
  set_modi &= ~SET_CHARGE;
}

void load_set() {
  File fp;
  SPIFFS.begin();
  if (SPIFFS.exists("/sets.txt")) {
    fp = SPIFFS.open("/sets.txt", "r");
    fp.read((uint8_t *)&sets, sizeof(sets));
    fp.close();
  }
  if (sets.crc32 != calculateCRC32((uint8_t*) &sets, sizeof(sets) - sizeof(sets.crc32))) {
    if (SPIFFS.exists("/sets_default.txt")) {
      fp = SPIFFS.open("/sets_default.txt", "r");
      fp.read((uint8_t *)&sets, sizeof(sets));
      fp.close();
    }
    if (sets.crc32 != calculateCRC32((uint8_t*) &sets, sizeof(sets) - sizeof(sets.crc32))) {
      sets.ac_i_calibration = 1.44 / (0.00199 * 1000);
      sets.ac_v_calibration = 1.881;
      sets.reserved0 = 0;
      sets.reserved1 = 0;
      sets.crc32 = calculateCRC32((uint8_t*) &sets, sizeof(sets) - sizeof(sets.crc32));
    }
    save_set();
  }
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
