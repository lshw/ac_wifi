
struct data {
  tm time;
  float kwh0;
  float kwh1;
  float i_max;
};
struct dataday {
  tm time; //4
  float kwh; //4
};

uint16_t data100ms_p = 0;
float data100ms[600]; //2400 bytes

struct datahour {
  float kwh;
  float power_max;
} mins_data[60]; //720 bytes

struct datahour hour_data[24]; //size 960字节
struct dataday day_data[180]; //size 1440字节 //180天

