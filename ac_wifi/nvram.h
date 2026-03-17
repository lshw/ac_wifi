#ifndef __NVRAM__
#define __NVRAM__

#define NVRAM7_URL 0b10
#define NVRAM7_UPDATE 0b1000

extern float datahour[24];
String ac_name;
double get_kwh();
#define SET_CHARGE 0b1
#include "calibration.h"
void update_kwh_count();  //更新kwh的脉冲数，
uint8_t set_modi = 0;     //不等于0, 需要保存
struct {
  uint8_t nc;
  uint8_t nvram7;
  uint8_t reserved;
  uint8_t ch;
  uint32_t ac_kwh_count = 0;  //几个脉冲一度电。
  double kwh;                 //总度数
  uint32_t ac_pf;             //未换算成度数的pf计数,  超过 sets.ac_kwh_count  就进1到ac_kwh
  uint32_t ac_pf0;            //已统计的HLW8032的pf 如果实际的小于这个数据， 就要把它加上8032的读数， 加到 sets.ac_pf, 并且设置本行为8032读数。
  double kwh_hour0 = -1.0;    //最后一个小时的kwh初值
  double kwh_day0 = -1.0;     //最后一天的kwh初值
  uint32_t reserved1[5];      //保留以后使用
  uint32_t crc32;
} __attribute__((packed)) nvram;
uint32_t nvram_save = 0;
struct {  //不会经常变化的设置， 需要保存到文件系统 sets.dat
  uint8_t on_off;
  uint8_t i_max;
  uint16_t serial;
  uint32_t color;
  float ac_v_calibration;
  float ac_i_calibration;
  uint32_t switch_on_time;   //ms
  uint32_t switch_off_time;  //ms
  uint16_t released1;
  uint16_t vol;  //音量
  float tz;      //时区
  char ntp[20];  //授时服务器
  uint32_t crc32;
} __attribute__((packed)) sets;  //字节紧凑格式， 不做字对齐

uint32_t calculateCRC32(const uint8_t *data, size_t length);

void save_nvram() {
  nvram.crc32 = calculateCRC32((uint8_t *)&nvram, sizeof(nvram) - sizeof(nvram.crc32));
  ESP.rtcUserMemoryWrite(0, (uint32_t *)&nvram, sizeof(nvram));
  nvram_save = millis() + 60000;  //60秒后 保存 nvram到 file
}

uint32_t last_save = 0;
void save_nvram_file() {
  File fp;
  if (nvram_save == 0) return;

  if (last_save < millis()) {  //最多120秒保存一次数据
    if (nvram_save > millis()
        && millis() - last_save < 12000
        && nvram_save - millis() < 600000)  //可能millis() 溢出
      return;
  }
  last_save = millis();
  if (SPIFFS.begin()) {
    fp = SPIFFS.open("/nvram.txt", "w");
    save_nvram();
    if (fp) {
      if (fp.write((uint8_t *)&nvram, sizeof(nvram)) == sizeof(nvram)) {
        nvram_save = 0;  // codex修改: 只有文件写成功后才清除待保存标志
      } else {
        Serial.println(F("nvram.txt写入不完整"));  // codex修改: RTC 数据落盘失败时输出日志，避免静默丢失持久化状态
      }
      fp.close();
    } else {
      Serial.println(F("打开nvram.txt失败"));  // codex修改: 补齐 NVRAM 文件句柄检查，避免空句柄写入
    }
    SPIFFS.end();
  } else {
    Serial.println(F("SPIFFS打开失败, 无法保存nvram"));  // codex修改: 文件系统不可用时明确提示持久化未执行
  }
}
void load_nvram() {
  File fp;
  ESP.rtcUserMemoryRead(0, (uint32_t *)&nvram, sizeof(nvram));
  if (nvram.crc32 != calculateCRC32((uint8_t *)&nvram, sizeof(nvram) - sizeof(nvram.crc32))) {
    bool spiffs_ok = SPIFFS.begin();
    if (spiffs_ok && SPIFFS.exists("/nvram.txt")) {
      fp = SPIFFS.open("/nvram.txt", "r");
      if (fp) {
        if (fp.read((uint8_t *)&nvram, sizeof(nvram)) != sizeof(nvram)) {
          memset(&nvram, 0, sizeof(nvram));  // codex修改: 文件长度不足时视为损坏，避免半截结构体参与 CRC
        }
        fp.close();
      }
    }
    if (nvram.crc32 != calculateCRC32((uint8_t *)&nvram, sizeof(nvram) - sizeof(nvram.crc32))) {
      memset(&nvram, 0, sizeof(nvram));
      update_kwh_count();  //校准数据初始化
      if (spiffs_ok && !SPIFFS.remove("/hours.dat")) {
        Serial.println(F("删除hours.dat失败"));  // codex修改: 小时统计损坏后清理失败时输出日志，避免旧文件残留无提示
      }
      memset(datahour, 0, sizeof(datahour));
    } else {
      fp = spiffs_ok ? SPIFFS.open("/hours.dat", "r") : File();
      if (fp) {
        if (fp.read((uint8_t *)&datahour, sizeof(datahour)) != sizeof(datahour)) {
          memset(datahour, 0, sizeof(datahour));  // codex修改: 小时统计不完整时直接丢弃，避免旧数据残留
        }
        fp.close();
      } else {
        memset(datahour, 0, sizeof(datahour));
      }
    }
    if (spiffs_ok) SPIFFS.end();
  } else {
    Serial.print(F("\r\nwifi channel="));
    Serial.println(nvram.ch);
    WRITE_PERI_REG(0x600011f4, 1 << 16 | nvram.ch);
  }

  nvram.kwh_hour0 = get_kwh();
  nvram.kwh_day0 = nvram.kwh_hour0;
  save_nvram();
}

void save_set(bool _default) {
  File fp;
  sets.crc32 = calculateCRC32((uint8_t *)&sets, sizeof(sets) - sizeof(sets.crc32));
  if (SPIFFS.begin()) {
    if (_default)
      fp = SPIFFS.open("/sets_default.txt", "w");
    else
      fp = SPIFFS.open("/sets.txt", "w");
    if (fp) {
      if (fp.write((uint8_t *)&sets, sizeof(sets)) == sizeof(sets)) {
        set_modi &= ~SET_CHARGE;  // codex修改: 写盘成功后再清除脏标记
      } else if (_default) {
        Serial.println(F("sets_default.txt写入不完整"));  // codex修改: 默认配置写盘失败时输出日志，避免静默丢失恢复基线
      } else {
        Serial.println(F("sets.txt写入不完整"));  // codex修改: 当前配置写盘失败时输出日志，避免静默丢配置
      }
      fp.close();
    } else if (_default) {
      Serial.println(F("打开sets_default.txt失败"));  // codex修改: 默认配置文件句柄检查
    } else {
      Serial.println(F("打开sets.txt失败"));  // codex修改: 当前配置文件句柄检查
    }
    SPIFFS.end();
  } else if (_default) {
    Serial.println(F("SPIFFS打开失败, 无法保存默认配置"));  // codex修改: 文件系统不可用时明确提示默认配置未落盘
  } else {
    Serial.println(F("SPIFFS打开失败, 无法保存配置"));  // codex修改: 文件系统不可用时明确提示当前配置未落盘
  }
}

void load_set() {
  File fp;
  bool spiffs_ok = SPIFFS.begin();
  bool need_save_set = false;
  if (ac_name == "") {
    if (spiffs_ok && SPIFFS.exists("/ac_name.txt")) {
      fp = SPIFFS.open("/ac_name.txt", "r");
      if (fp) {
        ac_name = fp.readString();
        ac_name.trim();
        fp.close();
      }
    }
  }
  if (spiffs_ok && SPIFFS.exists("/sets.txt")) {
    fp = SPIFFS.open("/sets.txt", "r");
    if (fp) {
      if (fp.read((uint8_t *)&sets, sizeof(sets)) != sizeof(sets)) {
        memset(&sets, 0, sizeof(sets));  // codex修改: 配置文件长度异常时按损坏处理
      }
      fp.close();
    }
  }
  uint32_t chipid = ESP.getChipId();
  if (sets.crc32 != calculateCRC32((uint8_t *)&sets, sizeof(sets) - sizeof(sets.crc32))) {
    if (spiffs_ok && SPIFFS.exists("/sets_default.txt")) {
      fp = SPIFFS.open("/sets_default.txt", "r");
      if (fp) {
        if (fp.read((uint8_t *)&sets, sizeof(sets)) != sizeof(sets)) {
          memset(&sets, 0, sizeof(sets));  // codex修改: 默认配置文件长度异常时按损坏处理
        }
        fp.close();
      }
    }
    if (sets.crc32 != calculateCRC32((uint8_t *)&sets, sizeof(sets) - sizeof(sets.crc32))) {
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
      sets.color = 0x0f00L;  //绿色
      sets.vol = 5;
      sets.tz = 8.0;
      sets.on_off = HIGH;  //默认关闭
      sets.switch_on_time = 0;
      sets.switch_off_time = 0;
    }
    need_save_set = true;  // codex修改: 退出当前 SPIFFS 会话后再保存，避免 begin/end 嵌套
  }
  for (uint16_t i = 0; i < sizeof(calibrations) / sizeof(calibration); i++) {
    if (chipid == calibrations[i].serial) {
      if (sets.i_max != calibrations[i].i_max) {
        sets.i_max = calibrations[i].i_max;
        need_save_set = true;  // codex修改: 合并成一次保存，减少文件系统反复开关
      }
    }
  }
  if (spiffs_ok) SPIFFS.end();
  if (need_save_set) save_set(false);
}

#endif  //__NVRAM__
