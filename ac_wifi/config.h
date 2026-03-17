#ifndef _CONFIG_H_
#define _CONFIG_H_

#define VER "2.1"
#define CRC_MAGIC 1
String hostname = "AC-";
#define NETLOG  //打开netlog
#define DEFAULT_URL0 "http://ac_wifi.cfido.com:808/ac_wifi.php"
#define DEFAULT_URL1 "http://ac_wifi.wf163.com:808/ac_wifi.php"
inline bool millis_reached(uint32_t deadline, uint32_t now_ms = millis()) {
  return (int32_t)(now_ms - deadline) >= 0;  // codex修改: 用有符号时间差比较 millis，避免 49.7 天溢出后定时判断反转
}
inline bool millis_before(uint32_t deadline, uint32_t now_ms = millis()) {
  return (int32_t)(now_ms - deadline) < 0;  // codex修改: 统一提供与 millis_reached 配套的溢出安全“未到期”判断
}

#define SSR 4      //SSD
#define KEYWORD 0  //按键

#endif  //_CONFIG_H_
