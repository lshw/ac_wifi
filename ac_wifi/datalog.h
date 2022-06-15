
struct dataday {
  time_t time; //4
  float kwh; //4
};

#define KWH_DAYS 100
dataday kwh_days[KWH_DAYS];
int8_t kwh_days_p = -1;

uint16_t data100ms_p = 0;
float data100ms[600]; //2400 bytes 瞬时功率
float datamins[60];//240 byte 每分钟最大功率
float datahour[24];//96字节  每一小时的耗电量

