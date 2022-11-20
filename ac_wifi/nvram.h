#ifndef __NVRAM__
#define __NVRAM__

#define NVRAM7_URL    0b10
#define NVRAM7_UPDATE 0b1000

extern float datahour[24];
String ac_name;

#define SET_CHARGE 0b1
#include "calibration.h"
void update_kwh_count(); //更新kwh的脉冲数，
uint8_t set_modi = 0; //不等于0, 需要保存
struct {
  uint8_t nc;
  uint8_t nvram7;
  uint8_t reserved;
  uint8_t ch;
  uint32_t ac_kwh_count = 0; //几个脉冲一度电。
  double  kwh; //总度数
  uint32_t  ac_pf; //未换算成度数的pf计数,  超过 sets.ac_kwh_count  就进1到ac_kwh
  uint32_t  ac_pf0; //已统计的HLW8032的pf 如果实际的小于这个数据， 就要把它加上8032的读数， 加到 sets.ac_pf, 并且设置本行为8032读数。
  double kwh_hour0 = -1.0; //最后一个小时的kwh初值
  double kwh_day0 = -1.0; //最后一天的kwh初值
  uint32_t reserved1[5]; //保留以后使用
  uint32_t crc32;
} __attribute__ ((packed)) nvram;
uint32_t nvram_save = 0;
struct { //不会经常变化的设置， 需要保存到文件系统 sets.dat
  uint8_t reserved0;
  uint8_t i_max;
  uint16_t serial;
  uint32_t color;
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

uint32_t last_save = 0;
void save_nvram_file() {
  File fp;
  if (nvram_save == 0) return;

  if (last_save  < millis()) { //最多120秒保存一次数据
    if (nvram_save > millis()
        && millis() - last_save < 12000
        && nvram_save - millis() < 600000) //可能millis() 溢出
      return;

  }
  last_save = millis();
  SPIFFS.begin();
  fp = SPIFFS.open("/nvram.txt", "w");
  save_nvram();
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
      if (fp) {
        fp.read((uint8_t *)&nvram, sizeof(nvram));
        fp.close();
      }
    }
    if (nvram.crc32 != calculateCRC32((uint8_t*) &nvram, sizeof(nvram) - sizeof(nvram.crc32))) {
      memset(&nvram, 0, sizeof(nvram));
      update_kwh_count(); //校准数据初始化
      SPIFFS.remove("/hours.dat");
      memset(datahour, 0, sizeof(datahour));
    } else {
      fp = SPIFFS.open("/hours.dat", "r");
      if (fp) {
        fp.read((uint8_t *)&datahour, sizeof(datahour));
        fp.close();
      }
    }
    SPIFFS.end();
    save_nvram();
  } else {
    Serial.print(F("\r\nwifi channel="));
    Serial.println(nvram.ch);
    WRITE_PERI_REG(0x600011f4, 1 << 16 | nvram.ch);
  }
}

void save_set(bool _default) {
  File fp;
  sets.crc32 = calculateCRC32((uint8_t*) &sets, sizeof(sets) - sizeof(sets.crc32));
  SPIFFS.begin();
  if (_default)
    fp = SPIFFS.open("/sets_default.txt", "w");
  else
    fp = SPIFFS.open("/sets.txt", "w");
  fp.write((uint8_t *)&sets, sizeof(sets));
  fp.close();
  SPIFFS.end();
  set_modi &= ~SET_CHARGE;
}

void load_set() {
  File fp;
  SPIFFS.begin();
  if (ac_name == "") {
    if (SPIFFS.exists("/ac_name.txt")) {
      fp = SPIFFS.open("/ac_name.txt", "r");
      ac_name = fp.readString();
      ac_name.trim();
      fp.close();
    }
  }
  if (SPIFFS.exists("/sets.txt")) {
    fp = SPIFFS.open("/sets.txt", "r");
    fp.read((uint8_t *)&sets, sizeof(sets));
    fp.close();
  }
  uint32_t chipid = ESP.getChipId();
  if (sets.crc32 != calculateCRC32((uint8_t*) &sets, sizeof(sets) - sizeof(sets.crc32))) {
    if (SPIFFS.exists("/sets_default.txt")) {
      fp = SPIFFS.open("/sets_default.txt", "r");
      fp.read((uint8_t *)&sets, sizeof(sets));
      fp.close();
    }
    if (sets.crc32 != calculateCRC32((uint8_t*) &sets, sizeof(sets) - sizeof(sets.crc32))) {
      sets.serial = 0;
      for (uint16_t i = 0; i < sizeof(calibrations) / sizeof(calibration); i++) {
        if (chipid == calibrations[i].serial) {
          sets.serial = i;
          sets.ac_i_calibration = calibrations[i].i;
          sets.ac_v_calibration = calibrations[i].v;
          sets.i_max = calibrations[i].i_max;
          break;
        }
      }
      sets.reserved0 = 0;
      sets.color = 0x0f00L; //绿色
    }
    save_set(false);
  }
  for (uint16_t i = 0; i < sizeof(calibrations) / sizeof(calibration); i++) {
    if (chipid == calibrations[i].serial) {
      if (sets.i_max != calibrations[i].i_max) {
        sets.i_max = calibrations[i].i_max;
        save_set(false);
      }
    }
  }
  SPIFFS.end();
}

#endif //__NVRAM__
