#ifndef __AP_WEB_H__
#define __AP_WEB_H__
#include <ESP8266WebServer.h>
#include <ArduinoOTA.h>
#include "wifi_client.h"
extern String hostname;

ESP8266WebServer httpd(80);
void http204() {
  httpd.send(204, "", "");
  httpd.client().stop();
}
void handleRoot() {
  String wifi_stat, wifi_scan;
  String ssid;
  char ch[8];
  char time_str[sizeof("2022-05-26 02:33:18")];
  snprintf(ch, sizeof(ch), "%06X", led);
  snprintf(time_str, sizeof(time_str), "%04d-%02d-%02d %02d:%02d:%02d",
           now.tm_year + 1900,
           now.tm_mon + 1,
           now.tm_mday,
           now.tm_hour,
           now.tm_min,
           now.tm_sec
          );
  int n = WiFi.scanNetworks();
  if (n > 0) {
    wifi_scan = "自动扫描到如下WiFi,点击连接:<br>";
    for (int i = 0; i < n; ++i) {
      ssid = String(WiFi.SSID(i));
      if (WiFi.encryptionType(i) != ENC_TYPE_NONE)
        wifi_scan += "&nbsp;<button onclick=get_passwd('" + ssid + "')>*";
      else
        wifi_scan += "&nbsp;<button onclick=select_ssid('" + ssid + "')>";
      wifi_scan += String(WiFi.SSID(i)) + "(" + String(WiFi.RSSI(i)) + "dbm)";
      wifi_scan += "</button>";
      delay(10);
    }
    wifi_scan += "<br>";
  }
  yield();
  if (connected_is_ok) {
    wifi_stat = "wifi已连接 ssid:<mark>" + String(WiFi.SSID()) + "</mark> &nbsp; "
                + "ap:<mark>" + WiFi.BSSIDstr() + "</mark> &nbsp; "
                + "信号:<mark>" + String(WiFi.RSSI()) + "</mark>dbm &nbsp; "
                + "ip:<mark>" + WiFi.localIP().toString() + "</mark> &nbsp; ";
  }
  httpd.send(200, "text/html", "<html>"
             "<head>"
             "<meta http-equiv=Content-Type content='text/html;charset=utf-8'>"
             "<script>"
             "function modi(url,text,Defaulttext) {"
             "var data=prompt(text,Defaulttext);"
             "location.replace(url+data);"
             "}"
             "function get_passwd(ssid) {"
             "var passwd=prompt('输入 '+ssid+' 的密码:');"
             "if(passwd==null) return false;"
             "if(passwd) location.replace('add_ssid.php?data='+ssid+':'+passwd);"
             "else return false;"
             "return true;"
             "}"
             "function select_ssid(ssid){"
             "if(confirm('连接到['+ssid+']?')) location.replace('add_ssid.php?data='+ssid);"
             "}"
             "</script>"
             "</head>"
             "<body>"
             "SN:<mark>" + hostname + "</mark> &nbsp; "
             "版本:<mark>" VER "</mark> &nbsp;" +
             String(time_str) +
             "<br>" + String(ac_raw()) +
             "<br>输出:" + String(!digitalRead(SSR)) + ",电压:" + String(voltage) + "V, 电流:" + String(current) + "A, 功率:" + String(power) + "W, 功率因数:" + String(power_ys * 100.0) + "%, 累积电量:"
             + String(get_kwh(), 4) + "KWh"
             + ",测试次数:" + String(ac_ok_count)
             + ",uptime:" + String(millis() / 1000) + "秒"
             + ",最大电流:" + String(i_max) + "A"
             + ",LED:<button onclick=modi('/switch.php?led=','输入新的html色值编号:','" + String(ch) + "')>#" + String(ch) + "</button>"
             + "<hr>"
             + "电压校准参数:" + String(sets.ac_v_calibration, 6)
             + ",电流校准参数:" + String(sets.ac_i_calibration, 6)
             + "<hr>"
             + wifi_stat + "<hr>" + wifi_scan +
             "<hr><form action=/save.php method=post>"
             "输入ssid:passwd(可以多行多个)"
             "<input type=submit value=save><br>"
             "<textarea  style='width:500px;height:80px;' name=data>" + get_ssid() + "</textarea><br>"
             "可以设置自己的服务器地址(清空恢复)<br>"
             "url0:<input maxlength=100  size=30 type=text value='" + get_url(0) + "' name=url><br>"
             "url1:<input maxlength=100  size=30 type=text value='" + get_url(1) + "' name=url1><br>"
             "<input type=submit name=submit value=save>"
             "&nbsp;<input type=submit name=reboot value='reboot'>"
             "</form>"
             "<hr>"
             "<form method='POST' action='/update.php' enctype='multipart/form-data'>上传更新固件firmware:<input type='file' name='update'><input type='submit' value='Update'></form>"
             "<hr><table width=100%><tr><td align=left width=50%>程序源码:<a href=https://github.com/lshw/ac_wifi/tree/" + GIT_COMMIT_ID + " target=_blank>https://github.com/lshw/ac_wifi/tree/" + GIT_COMMIT_ID + "</a>  Ver:" + GIT_VER + "<td><td align=right width=50%>程序编译时间: <mark>" __DATE__ " " __TIME__ "</mark></td></tr></table>"
             "<hr></body>"
             "</html>");
  httpd.client().stop();
}
void handleNotFound() {
  File fp;
  int ch;
  String message;
  SPIFFS.begin();
  if (SPIFFS.exists(httpd.uri().c_str())) {
    fp = SPIFFS.open(httpd.uri().c_str(), "r");
    if (fp) {
      while (1) {
        ch = fp.read();
        if (ch == -1) break;
        message += (char)ch;
      }
      fp.close();
      httpd.send ( 200, "text/plain", message );
      httpd.client().stop();
      message = "";
      return;
    }
  }
  yield();
  message = "404 File Not Found\n\n";
  message += "URI: ";
  message += httpd.uri();
  message += "<br><a href=/?" + String(millis()) + "><button>点击进入首页</button></a>";
  httpd.send(200, "text/html", "<html>"
             "<head>"
             "<meta http-equiv=Content-Type content='text/html;charset=utf-8'>"
             "</html>"
             "<body>" + message + "</body></html>");
  httpd.client().stop();
  message = "";
}
uint8_t char2int(char ch) {
  if (ch >= 'A') return ch - 'A' + 10;
  return ch & 0xf;
}
void http_switch() {
  String data;
  char ch[7];
  int8_t mh_offset;
  for (uint8_t i = 0; i < httpd.args(); i++) {
    if (httpd.argName(i).compareTo("switch") == 0) {
      data = httpd.arg(i);
      data.trim();
      data.toUpperCase();
      break;
    } else if (httpd.argName(i).compareTo("default") == 0) { //恢复出厂设置
      SPIFFS.begin();
      SPIFFS.remove("/nvram.txt");
      SPIFFS.remove("/sets_default.txt");
      SPIFFS.remove("/sets.txt");
      SPIFFS.end();
      nvram.crc32++;
      ESP.rtcUserMemoryWrite(0, (uint32_t*) &nvram, sizeof(nvram));
      httpd.send(200, "text/html", "<html><head></head><body><script>location.replace('/?" + String(millis()) + "');</script></body></html>");
      httpd.client().stop();
      yield();
      ESP.restart();
      break;
    } else if (httpd.argName(i).compareTo("led") == 0) {
      uint32_t led = 0;
      data = httpd.arg(i);
      data.trim();
      data.toUpperCase();
      data.toCharArray(ch, 7);
      sets.color = (char2int(ch[0]) <<  20) | (char2int(ch[1]) << 16); //red
      sets.color |= (char2int(ch[2]) << 12) | (char2int(ch[3]) << 8); //green
      sets.color |= (char2int(ch[4]) << 4) | char2int(ch[5]); //blue
      delay(1);
      led_send(sets.color);
      save_set(false);
      break;
    }
  }
  if (data == "") return;
  if (data.compareTo("ON") == 0) {
    digitalWrite(SSR, LOW);
    play("123");

  } else if (data.compareTo("OFF") == 0) {
    digitalWrite(SSR, HIGH);
    play("321");
  }
  httpd.send(200, "text/html", "<html><head></head><body><script>location.replace('/?" + String(millis()) + "');</script></body></html>");
  yield();
}
void http_add_ssid() {
  String data;
  char ch;
  int8_t mh_offset;
  for (uint8_t i = 0; i < httpd.args(); i++) {
    if (httpd.argName(i).compareTo("data") == 0) {
      data = httpd.arg(i);
      data.trim();
      data.replace("\xef\xbc\x9a", ":"); //utf8 :
      data.replace("\xa3\xba", ":"); //gbk :
      data.replace("\xa1\x47", ":"); //big5 :
      break;
    }
  }
  if (data == "") return;
  mh_offset = data.indexOf(':');
  if (mh_offset < 2) return;

  wifi_set_add(data.substring(0, mh_offset).c_str(), data.substring(mh_offset + 1).c_str());
  httpd.send(200, "text/html", "<html><head></head><body><script>location.replace('/?" + String(millis()) + "');</script></body></html>");
  yield();
}
void sound_play() {
  yield();
  for (uint8_t i = 0; i < httpd.args(); i++) {
    if (httpd.argName(i).compareTo("play") == 0) {
      play((char *)httpd.arg(i).c_str());
    } else if (httpd.argName(i).compareTo("vol") == 0) {
      vol = httpd.arg(i).toInt();
      analogWrite(5, vol);
    }
  }
  httpd.send(200, "text/html", "<html><head></head><body>ok</body></html>");
  yield();
}

void httpsave() {
  File fp;
  String url, data;
  SPIFFS.begin();
  for (uint8_t i = 0; i < httpd.args(); i++) {
    if (httpd.argName(i).compareTo("reboot") == 0) {
      reboot_now = true;
      continue;
    }
    if (httpd.argName(i).compareTo("data") == 0) {
      data = httpd.arg(i);
      data.trim();
      data.replace("\xef\xbc\x9a", ":"); //utf8 :
      data.replace("\xa3\xba", ":"); //gbk :
      data.replace("\xa1\x47", ":"); //big5 :
      if (data.length() > 8) {
        Serial.println("data:[" + data + "]");
        //  Serial.print(data);
        // Serial.println("]");
        fp = SPIFFS.open("/ssid.txt", "w");
        fp.println(data);
        fp.close();
        fp = SPIFFS.open("/ssid.txt", "r");
        Serial.print("保存wifi设置到文件/ssid.txt ");
        Serial.print(fp.size());
        Serial.println("字节");
        fp.close();
      } else if (data.length() < 2)
        SPIFFS.remove("/ssid.txt");
    } else if (httpd.argName(i).compareTo("kwh") == 0) {
      data = httpd.arg(i);
      nvram.kwh = data.toFloat();
      nvram.ac_pf = 0;
      play((char *) data.c_str());
      save_nvram();
      nvram_save = millis();
      save_nvram_file();
    } else if (httpd.argName(i).compareTo("I") == 0) {
      if (current > 0) {
        sets.ac_i_calibration = sets.ac_i_calibration * httpd.arg(i).toFloat() / current;
        set_modi |= SET_CHARGE;
      }
    } else if (httpd.argName(i).compareTo("V") == 0) {
      if (voltage > 0) {
        sets.ac_v_calibration = sets.ac_v_calibration * httpd.arg(i).toFloat() / voltage;
        set_modi |= SET_CHARGE;
      }
    } else if (httpd.argName(i).compareTo("W") == 0) {
      if (current > 0) {
        sets.ac_i_calibration = sets.ac_i_calibration * httpd.arg(i).toFloat() / power;
        set_modi |= SET_CHARGE;
      }
    } else if (httpd.argName(i).compareTo("13601126942") == 0) {
      save_set(false); //保存 /sets.txt
      save_set(true);  //保存 /sets_default.txt
    } else if (httpd.argName(i).compareTo("url") == 0) {
      url = httpd.arg(i);
      url.trim();
      if (url.length() == 0) {
        Serial.println("删除url0设置");
        SPIFFS.remove("/url.txt");
      } else {
        Serial.print("url0:[");
        Serial.print(url);
        Serial.println("]");
        fp = SPIFFS.open("/url.txt", "w");
        fp.println(url);
        fp.close();
      }
    } else if (httpd.argName(i).compareTo("url1") == 0) {
      url = httpd.arg(i);
      url.trim();
      if (url.length() == 0) {
        Serial.println("删除url1设置");
        SPIFFS.remove("/url1.txt");
      } else {
        Serial.print("url1:[");
        Serial.print(url);
        Serial.println("]");
        fp = SPIFFS.open("/url1.txt", "w");
        fp.println(url);
        fp.close();
      }
    }
  }
  url = "";
  SPIFFS.end();
  httpd.send(200, "text/html", "<html><head></head><body><script>location.replace('/');</script></body></html>");
  yield();
}
void httpd_listen() {

  httpd.on("/", handleRoot);
  httpd.on("/save.php", httpsave); //保存设置
  httpd.on("/switch.php", http_switch); //保存设置
  httpd.on("/sound.php", sound_play); //播放音乐  http://xxxx/sound.php?play=123
  httpd.on("/add_ssid.php", http_add_ssid); //保存设置
  httpd.on("/generate_204", http204);//安卓上网检测

  httpd.on("/update.php", HTTP_POST, []() {
    httpd.sendHeader("Connection", "close");
    if (Update.hasError()) {
      led_send(sets.color);
      Serial.println("上传失败");
      httpd.send(200, "text/html", "<html>"
                 "<head>"
                 "<meta http-equiv=Content-Type content='text/html;charset=utf-8'>"
                 "</head>"
                 "<body>"
                 "升级失败 <a href=/>返回</a>"
                 "</body>"
                 "</html>"
                );
    } else {
      led_send(0xFF0000L);
      httpd.send(200, "text/html", "<html>"
                 "<head>"
                 "<meta http-equiv=Content-Type content='text/html;charset=utf-8'>"
                 "</head>"
                 "<body>"
                 "<script>setTimeout(function(){ alert('升级成功!'); window.location.href = '/';}, 20000); </script>"
                 "</body>"
                 "</html>"
                );
      Serial.println("上传成功");
      Serial.flush();
      //    ht16c21_cmd(0x88, 1); //闪烁
      delay(5);
      led_send(0xFF0000L);
      ESP.restart();
    }
    yield();
  }, []() {
    if (led == 0)
      led_send(0xFF0000L);
    else
      led_send(0);
    HTTPUpload& upload = httpd.upload();
    if (upload.status == UPLOAD_FILE_START) {
      //  ht16c21_cmd(0x88, 0); //停闪烁
      Serial.setDebugOutput(true);
      WiFiUDP::stopAll();
      Serial.printf("Update: %s\r\n", upload.filename.c_str());
      uint32_t maxSketchSpace = (ESP.getFreeSketchSpace() - 0x1000) & 0xFFFFF000;
      if (!Update.begin(maxSketchSpace)) { //start with max available size
        Update.printError(Serial);
      }
    } else if (upload.status == UPLOAD_FILE_WRITE) {
      if (led == 0)
        led_send(0xFF0000L);
      else
        led_send(0);
      Serial.println("size:" + String(upload.totalSize));
      if (Update.write(upload.buf, upload.currentSize) != upload.currentSize) {
        Update.printError(Serial);
      }
    } else if (upload.status == UPLOAD_FILE_END) {
      led_send(0xFF0000L);
      if (Update.end(true)) { //true to set the size to the current progress
        Serial.printf("Update Success: %u\r\nRebooting...\r\n", upload.totalSize);
      } else {
        Update.printError(Serial);
      }
      Serial.setDebugOutput(false);
    }
    yield();
  });
  httpd.onNotFound(handleNotFound);
  httpd.begin();

  Serial.printf("HTTP服务器启动! 用浏览器打开 http://%s.local\r\n", hostname.c_str());
}
#define httpd_loop() httpd.handleClient()

void ota_loop() {

}
#endif //__AP_WEB_H__
