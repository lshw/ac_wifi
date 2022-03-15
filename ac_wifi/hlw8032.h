#ifndef __HLW8032_H__
#define __HLW8032_H__
uint8_t ac_buf[25];
char ac_str[100];
uint32_t last_ac = 0;
int32_t pf = -1;
float current = 0.0, voltage = 0.0, power = 0.0, power_ys = 0.0; //上次测量
uint32_t v_cs = 0, i_cs = 0, p_cs = 0;
float voltage0 = 0.0; //上次测量的电压值
struct sets {
  uint8_t bz; //最高第7位固定为0作为起始标志，在ram中，最低一位为更新标志， 设为1为已更新，需要保存数据 ,后面的结构， 根据项目需要定义
  double  kwh; //总度数
  uint32_t  ac_pf; //未换算成度数的pf计数,  超过 sets.ac_kwh_count  就进1到ac_kwh
  uint32_t  ac_pf0; //已统计的HLW8032的pf 如果实际的小于这个数据， 就要把它加上8032的读数， 加到 sets.ac_pf, 并且设置本行为8032读数。
  uint16_t  count;
  uint16_t ac_power_change_value;//429,430
  uint16_t ac_voltage_change_value;//431,432
  uint16_t ac_alert_minute;//433,434
  uint32_t ac_kwh_count;//435,436,437,438
  float ac_v_calibration;//439,440,441,442
  float ac_i_calibration;//443,444,445,446
} __attribute__ ((packed)); //字节紧凑格式， 不做字对齐
struct sets sets;
/*
  double get_kwh(){
   double kwh;
   kwh = sets.kwh;
   if(sets.ac_kwh_count > 0)
     kwh += (double)sets.ac_pf/sets.ac_kwh_count;
   return kwh;
  }

  void update_pf(){
  if(pf < 0) return; //hlm8032未开始工作
  if(sets.ac_pf0 == pf) return;
  if(sets.ac_pf0 < pf) {
    sets.ac_pf += (pf - sets.ac_pf0);
  }
   sets.ac_pf0 = pf;
   if(sets.ac_kwh_count > 0 && sets.ac_pf >= sets.ac_kwh_count) {
      sets.kwh += 1.0;
      sets.ac_pf -= sets.ac_kwh_count;
   }
   sets.bz |= 0x1;
  }

  void update_kwh_count() {
      uint32_t new_kwh_count;
      if(p_cs == 0) {
        sets.ac_kwh_count = 0; //无效
        return;
      }
      new_kwh_count = (uint32_t) 1000000000 / (p_cs * sets.ac_i_calibration * sets.ac_v_calibration / 3600); //HLW8032手册15页
      if(new_kwh_count != sets.ac_kwh_count && new_kwh_count > 50000 && new_kwh_count < 2000000) {//0.5m欧-10m欧
      update_pf();
      sets.kwh += (double)(sets.ac_pf / sets.ac_kwh_count);
      sets.ac_pf = 0;
      sets.ac_kwh_count = new_kwh_count;
      sets.bz |= 0x1;
      save_sets();
    }
  }

  void fix_ac_set() {
    //电阻采样4个470k 加1个1k为1.881  //参数=0.0001*(R1+R2)/R2  R1=470k*4,R2=1K
    if(!(sets.ac_v_calibration > 1.0 && sets.ac_v_calibration < 3.00)) sets.ac_v_calibration = 1.881;
    //1m欧->1.0 ,10m欧->0.1,  0.5m欧 -> 2.0 1.81*1.1m欧->0.50226 // 参数=0.0001/R
    if(!(sets.ac_i_calibration > 0.1 && sets.ac_i_calibration < 2.00)) sets.ac_i_calibration = 1.0 / (0.00199 * 1000);
    if(ac_int32(&ac_buf[2 + 6 + 6])!=0)
     update_kwh_count();
  }

  void ac_datalog_put(char * msg) {
    char buf[100];
    get_time();
    if(time_str[2]=='0') return; //时间不对
    update_pf();
    snprintf(buf,sizeof(buf),"%.2f,%.2f,%.2f,%.2f,%lf,%s", voltage, current*1000.0, power, power_ys , get_kwh(),msg);
    datalog.qz = 0x1f << 1; //raw asc格式
    datalog_put(buf,strlen(buf));
  }

*/
uint32_t ac_int32( uint8_t * dat) {
  uint32_t ret = 0;
  ret = (uint32_t) (dat[0] << 16) | (dat[1] << 8) | dat[2] ;
  return ret;
}
void get_ac() {
  uint32_t d32;
  float f1;
  uint8_t i = 0;
  int16_t dat, adc0, adc1;
  uint8_t state, dataUpdata;
  if (Serial.available() < 24) return;
  if (Serial.available()  > 24) {
    while (Serial.available() > 24)
      Serial.read();
    return;
  }

  memset(ac_buf, 0, sizeof(ac_buf));
  Serial.readBytes(ac_buf, 24);
  adc0 = 0;
  for (i = 2; i < 23; i++) {
    adc0 += ac_buf[i];
  }
  if ((adc0 & 0xff) != ac_buf[23]) {
    return;
  }

  last_ac = millis();
  pf = (uint32_t) ((ac_buf[20] & 0x80) << 9 | ( ac_buf[21] << 8) | ac_buf[22]);
  state = ac_buf[0];
  dataUpdata = ac_buf[20];
  if ((state & 0xf9) == 0xf8) voltage = 0.0; //电压
  else if (state == 0x55 || (state & 0b1001) == 0) {
    d32 = ac_int32(&ac_buf[2 + 3]);
    if ((d32 & 0xffff00) != 0) { // 滤除异常值
      v_cs = ac_int32(&ac_buf[2]);
      voltage = (float) v_cs / d32 * sets.ac_v_calibration;
      if (isnan(voltage) || isinf(voltage)) voltage = 0.0;
    }
  }
  /*
      if ((state & 0xf5) == 0xf4) current = 0.0;
      else if (state == 0x55 || (state & 0b101) == 0) {
        d32 = ac_int32(&ac_buf[8 + 3]);
        if ((d32 & 0xffff00) != 0) { // 滤除异常值
          i_cs = ac_int32(&ac_buf[8]);
          current =  (float) i_cs / d32 * sets.ac_i_calibration;
          if (isnan(current) || isinf(current)) current = 0.0;
        }
      }

      if ((state & 0xf3) == 0xf2) power = 0.0;
      else if (state == 0x55 || (state & 0b11) == 0) {
        d32 = ac_int32(&ac_buf[14 + 3]);
        if ((d32 & 0xffff00) != 0) { // 滤除异常值
          p_cs = ac_int32(&ac_buf[14]);
          power = (float) p_cs / d32 * sets.ac_v_calibration * sets.ac_i_calibration;
          if (isnan(power) || isinf(power)) power = 0.0;
        }
      }
      if (state == 0x55 || (state & 0b1111) == 0) {
        if ( current > 0.0 && voltage > 0.0) {
          f1 = power / current / voltage;
          if (f1 <= 1.0) power_ys = f1;
        } else power_ys = 0;
      }
      bool alert = ac_alert();
      if (ac_init == false && p_cs > 0) {
        fix_ac_set();
        ac_init = true;
      }
      update_pf();
      if (p_cs > 0 && sets.ac_kwh_count == 0) update_kwh_count();
  */
}

char * ac_raw() {
  uint8_t i = 0;
  memset(ac_str, 0, sizeof(ac_str));
  for (uint8_t i0 = 0; i0 < 24; i0++) {
    if (i0 == 1
        || i0 == 2
        || i0 == 5
        || i0 == 8
        || i0 == 11
        || i0 == 14
        || i0 == 17
        || i0 == 20
        || i0 == 21
        || i0 == 23 ) {
      ac_str[i] = ' ';
      i++;
    }
    snprintf(&ac_str[i], sizeof(ac_str), "%02x", ac_buf[i0]);
    i = i + 2;
  }
  return ac_str;
}
#endif //__HLW8032_H__

