
struct dataday {
  time_t time; //4
  float kwh; //4
} dataday; //日数据

uint16_t data100ms_p = 0;
float data100ms[600]; //2400 bytes 瞬时功率
float datamins[60];//240 byte 每分钟最大功率
float datahour[24];//96字节  每一小时的耗电量
