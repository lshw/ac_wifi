#ifndef __HLW8032_H__
#define __HLW8032_H__
uint8_t ac_buf[30];
char ac_str[120];
uint32_t last_ac = 0;
int32_t pf = -1;
bool ac_init = false;
float current = 0.0, voltage = 0.0, power = 0.0, power_ys = 0.0; //上次测量
uint32_t v_cs = 0, i_cs = 0, p_cs = 0;
uint32_t ac_kwh_count = 0; //开机后计算得到， 几个脉冲一度电。
float voltage0 = 0.0; //上次测量的电压值

uint32_t ac_int32( uint8_t * dat) { //从hlw8032的数据中获取32位整数
  uint32_t ret = 0;
  ret = (uint32_t) (dat[0] << 16) | (dat[1] << 8) | dat[2] ;
  return ret;
}

double get_kwh() { //获取当前数据
  double kwh;
  kwh = nvram.kwh;
  if (ac_kwh_count > 0)
    kwh += (double)nvram.ac_pf / ac_kwh_count;
  return kwh;
}

void update_pf() { //更新kwh累计， 清理脉冲计数
  if (pf < 0) return; //hlm8032未开始工作
  if (nvram.ac_pf0 == pf) return;
  if (nvram.ac_pf0 < pf) {
    nvram.ac_pf += (pf - nvram.ac_pf0);
  }
  nvram.ac_pf0 = pf;
  if (ac_kwh_count > 0 && nvram.ac_pf >= ac_kwh_count) {
    nvram.kwh += 1.0;
    nvram.ac_pf -= ac_kwh_count;
  }
  set_modi |= NVRAM_CHARGE;
  set_file_modi |= NVRAM_CHARGE;
}

void update_kwh_count() { //根据需要修改并保存校准数据
  uint32_t new_kwh_count;
  //电阻采样4个470k 加1个1k为1.881  //参数=0.0001*(R1+R2)/R2  R1=470k*4,R2=1K
  if (!(sets.ac_v_calibration > 1.0 && sets.ac_v_calibration < 3.00)) {
    sets.ac_v_calibration = 1.881;
    set_modi |= SET_FILE_CHARGE;
  }
  //1m欧->1.0 ,10m欧->0.1,  0.5m欧 -> 2.0 1.81*1.1m欧->0.50226 // 参数=0.0001/R
  if (!(sets.ac_i_calibration > 0.1 && sets.ac_i_calibration < 2.00)) {
    sets.ac_i_calibration = 1.44 / (0.00199 * 1000);
    set_modi |= SET_FILE_CHARGE;
  }
  if (p_cs == 0) {
    ac_kwh_count = 0; //无效
    return;
  }
  new_kwh_count = (uint32_t) 1000000000 / (p_cs * sets.ac_i_calibration * sets.ac_v_calibration / 3600); //HLW8032手册15页
  if (new_kwh_count != ac_kwh_count && new_kwh_count > 50000 && new_kwh_count < 2000000) { //0.5m欧-10m欧
    update_pf();
    if (ac_kwh_count > 0)
      nvram.kwh += (double)(nvram.ac_pf / ac_kwh_count);
    nvram.ac_pf = 0;
    ac_kwh_count = new_kwh_count;
    set_modi |= NVRAM_CHARGE;
    set_file_modi |= NVRAM_CHARGE;
  }
}

bool power_down = false;
bool ac_ok = false;
uint32_t ac_ok_count = 0;
void ac_20ms() {
  if (Serial.available() < 24) return;
  if (Serial.available()  > 24) {
    while (Serial.available() > 24) {
      Serial.read();
    }
    return;
  }
  memset(ac_buf, 0, sizeof(ac_buf));
  Serial.read(ac_buf, 24);
  for (uint8_t i = 2; i < 23; i++) {
    ac_buf[23] -= ac_buf[i];
  }
  if (ac_buf[23] != 0) {
    return;
  }
  last_ac = millis();
  ac_ok = true;
  ac_ok_count++;
}
uint32_t p_cs0 = 0;
void ac_decode() { //hlm8032数据解码
  uint32_t d32;
  float f1;
  uint8_t i = 0;
  uint8_t state, dataUpdata;
  if (!ac_ok) return;
  ac_ok = false;
  pf = (uint32_t) ((ac_buf[20] & 0x80) << 9 | ( ac_buf[21] << 8) | ac_buf[22]);
  state = ac_buf[0];
  dataUpdata = ac_buf[20];
  if ((state & 0xf5) == 0xf4) current = 0.0;
  else if (state == 0x55 || (state & 0b101) == 0) {
    d32 = ac_int32(&ac_buf[8 + 3]);
    if ((d32 & 0xffff00) != 0) { // 滤除异常值
      i_cs = ac_int32(&ac_buf[8]);
      current =  (float) i_cs / d32 * sets.ac_i_calibration;
      if (isnan(current) || isinf(current)) current = 0.0;
      else if (current > 5.0) { //过流保护
        digitalWrite(SSR, LOW);
        i_over = 10000; //关机10秒
      }
    }
  }

  if ((state & 0xf9) == 0xf8) voltage = 0.0; //电压
  else if (state == 0x55 || (state & 0b1001) == 0) {
    d32 = ac_int32(&ac_buf[2 + 3]);
    if ((d32 & 0xffff00) != 0) { // 滤除异常值
      v_cs = ac_int32(&ac_buf[2]);
      voltage = (float) v_cs / d32 * sets.ac_v_calibration;
      if (isnan(voltage) || isinf(voltage)) voltage = 0.0;
    }
  }

  if ((state & 0xf3) == 0xf2) power = 0.0;
  else if (state == 0x55 || (state & 0b11) == 0) {
    d32 = ac_int32(&ac_buf[14 + 3]);
    if ((d32 & 0xffff00) != 0) { // 滤除异常值
      p_cs = ac_int32(&ac_buf[2 + 6 + 6]);
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
  if (ac_init == false && p_cs > 0) {
    update_kwh_count();
    ac_init = true;
  }
  update_pf();
  if (power_down == false) {
    if (voltage < 40) {
      power_down = true;
      set_modi |= NVRAM_CHARGE;
      set_file_modi |= NVRAM_CHARGE;
    }
  } else if (voltage > 80) {
    power_down = false;
  }
  if (p_cs0 != p_cs) {
    p_cs0 = p_cs;
    update_kwh_count();
  }
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

