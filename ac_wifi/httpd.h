#ifndef __AP_WEB_H__
#define __AP_WEB_H__
#include <ESP8266WebServer.h>
#include <ArduinoOTA.h>
#include "wifi_client.h"
#include "global.h"
extern String hostname;
String body;
ESP8266WebServer httpd(80);
void httpd_send_200(String javascript) {
  httpd.sendHeader( "charset", "utf-8" );
  httpd.send(200, "text/html", "<html>"
             "<head>"
             "<title>" + ac_name + " " + GIT_VER + "</title>"
             "<meta http-equiv=Content-Type content='text/html;charset=utf-8'>"
             "<script>"
             "function modi(url,text,Defaulttext) {"
             "var data=prompt(text,Defaulttext);"
             "if (data==null) {return false;}"
             "location.replace(url+data);"
             "}"
             + javascript +
             "</script>"
             "</head>"
             "<body bgcolor='#ffffff'>"
             + body +
             "</body>"
             "</html>");
  httpd.client().stop();
}
void http204() {
  httpd.send(204, "", "");
  httpd.client().stop();
}
String switch_mode(uint16_t t) {
  if (t == 0)
    return String(F("保持"));
  return String(t) + F(" 秒");
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
  //body.reserve(8192 + 2048);
  body = "name:<mark onclick=modi('/save.php?ac_name=','修改标识?','" + ac_name + "')>" + ac_name + "</mark> &nbsp;"
         "SN:<mark>" + hostname + "</mark> &nbsp;"
         "版本:<mark>" VER "</mark> &nbsp;" +
         String(time_str) +
         "<br>" + String(ac_raw()) +
         "<br>开关状态:";
  if (digitalRead(SSR) == HIGH)  body += "<button onclick=gotoif('/save.php?switch=on','输出开启?');>关闭</button>";
  else body += F("<button onclick=gotoif('/save.php?switch=off','输出关闭?');>开启</button>");
  body += String(switch_change_time)
          + F("秒, 开机时长:<mark onclick=modi('/save.php?switch_on_time=','修改开机秒数,0为保持','") + String(sets.switch_on_time) + F("')>") + switch_mode(sets.switch_on_time) + F("</mark>&nbsp;&nbsp;"
              "关机时长:<mark onclick=modi('/save.php?switch_off_time=','修改关机秒数,0为保持','") + String(sets.switch_off_time) + F("')>") + switch_mode(sets.switch_off_time) + F("</mark><br>"
                  "电压:") + String(voltage) + F("V, 电流:") + String(current) + F("A, 功率:") + String(power) + F("W, 功率因数:") + String(power_ys * 100.0) + F("%, 累积电量:")
          + String(get_kwh(), 8) + F("KWh"
                                     ",测试次数:") + String(ac_ok_count)
          + F(",uptime:") + String(millis() / 1000) + F("秒"
              ",最大电流:") + String(i_max) + F("A"
                  ",LED:<button onclick=modi('/save.php?led=','输入新的html色值编号:','") + String(ch) + F("')>#") + String(ch) + F("</button>"
                      "<hr>"
                      "电压校准参数:") + String(sets.ac_v_calibration, 6)
          + F(",电流校准参数:") + String(sets.ac_i_calibration, 6)
          + F("<hr>");
  if (connected_is_ok) {
    body += "wifi已连接 ssid:<mark>" + String(WiFi.SSID()) + "</mark> &nbsp;"
            + "ap:<mark>" + WiFi.BSSIDstr() + "</mark> &nbsp;"
            + "信号:<mark>" + String(WiFi.RSSI()) + "</mark>dbm &nbsp;"
            + "ip:<mark>" + WiFi.localIP().toString() + "</mark><hr>";
  }
  for (uint8_t i = 0; i < httpd.args(); i++) {
    if (httpd.argName(i).compareTo("scan") == 0) {
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
    }
  }
  if (wifi_scan == "") {
    wifi_scan = "<a href=/?scan=1><buttom>扫描WiFi</buttom></a>";
  }
  body += wifi_scan + "<ht>";
  yield();

  body += "<form action=/save.php method=post>"
          "输入ssid:passwd(可以多行多个)"
          "<input type=submit value=save><br>"
          "<textarea  style='width:500px;height:80px;' name=data>" + get_ssid() + "</textarea><br>"
          "可以设置自己的服务器地址(清空恢复)<br>"
          "url0:<input maxlength=100  size=30 type=text value='" + get_url(0) + "' name=url><br>"
          "url1:<input maxlength=100  size=30 type=text value='" + get_url(1) + "' name=url1><br>"
          "<input type=submit name=submit value=save>"
          "&nbsp;<input type=submit name=reboot value='reboot'>"
          "</form>"
          "&nbsp;<input type=submit onclick=\"modi('/save.php?default=','输入恢复出厂设置的密码(其实就是SN号):','AC_')\" value='恢复出厂设置' title='密码:SN'>"
          "<hr>"
          "<div style='width: 700px; height: 400px; background-color: #606060; background-size: 100% 100%' id='power_sec'></div>"
          "<hr>"
          "<div style='width: 700px; height: 400px; background-color: #606060; background-size: 100% 100%' id='power_min'></div>"
          "<hr>"
          "<div style='width: 700px; height: 400px; background-color: #00a0a0; background-size: 100% 100%' id='wh_hour'></div>"
          "<hr>"
          "<div style='width: 700px; height: 400px; background-color: #00a0a0; background-size: 100% 100%' id='kwh_day'></div>"
          "<hr>"
          "<form method='POST' action='/update.php' enctype='multipart/form-data'>上传更新固件firmware:<input type='file' name='update'><input type='submit' value='Update'></form>"
          "<hr><table width=100%><tr><td align=left width=50%>程序源码:<a href=https://github.com/lshw/ac_wifi/tree/" + GIT_COMMIT_ID + " target=_blank>https://github.com/lshw/ac_wifi/tree/" + GIT_COMMIT_ID + "</a>  Ver:" + GIT_VER + "<td><td align=right width=50%>程序编译时间: <mark>" __DATE__ " " __TIME__ "</mark></td></tr></table>"
          "<script>\
var obj = {\
id:'power_sec',\
width:700,\
height:400,\
datas:[\
{\
name:'功率(W)',\
color:'red',\
data:[";
  for (uint16_t i = 0; i < 600; i++) {
    body += String(data100ms[(i + data100ms_p) % 600], 1);
    body += ",";
  }
  body += "]\
}\
],\
startX:40,\
startY:380,\
labelColor:'white',\
labelCount:10,\
nameSpace : 1,\
circleColor:'blue',\
tip:'最近60秒的功率曲线'\
};\
drawLine(obj);\
obj.id='power_min';\
obj.nameSpace=10;\
obj.datas=[{\
name:'功率(W)',\
color:'red',\
data:[";
  for (uint16_t i = 0; i < 60; i++) {
    body += String(datamins[(now.tm_min + i + 1) % 60]);
    body += ",";
  }
  body += "]}];\
obj.tip='最近1小时功率曲线',\
drawLine(obj);\
obj.id='wh_hour';\
obj.nameSpace=25;\
obj.datas=[{\
name:'耗电量(Wh)',\
color:'red',\
data:[";
  for (uint16_t i = 0; i < 24; i++) {
    body += String(datahour[(now.tm_hour + i + 1) % 24] * 1000.0);
    body += ",";
  }
  body += "]}];\
obj.tip='最近24小时的耗电量曲线';\
drawLine(obj);\
obj.id='kwh_day';\
obj.nameSpace=6;\
obj.datas=[{\
name:'耗电量(KWh)',\
color:'red',\
data:[";
  for (uint16_t i = 0; i < KWH_DAYS; i++) {
    body += String(kwh_days[(kwh_days_p + i) % KWH_DAYS].kwh, 4);
    body += ",";
  }
  body += "]}];\
obj.tip='日耗电量曲线';\
drawLine(obj);\
</script>";

  httpd_send_200(
    "function drawLine(obj) {\
var id = obj.id;\
var datas = obj.datas;\
var width = obj.width;\
var height = obj.height;\
var startX = obj.startX;\
var startY = obj.startY;\
var labelColor = obj.labelColor;\
var labelCount = obj.labelCount;\
var nameSpace = obj.nameSpace;\
var tip = obj.tip;\
var circleColor = obj.circleColor;\
\
var container = document.getElementById(id);\
var canvas = document.createElement('canvas');\
canvas.width = width;\
canvas.height = height;\
canvas.style.border = '1px solid red';\
container.appendChild(canvas);\
var cvs = canvas.getContext('2d');\
cvs.beginPath();\
cvs.strokeStyle = 'white';\
var startY1 = 20;\
cvs.moveTo(startX, startY1);\
cvs.lineTo(startX, startY);\
cvs.lineTo(700, startY);\
cvs.stroke();\
var length = datas.length;\
var length1 = datas[0].data.length;\
var maxNum = 0;\
for(var i = 0;i < length;i++){\
for (var j = 0;j < length1;j++){\
if (maxNum <= datas[i].data[j]) {\
maxNum = datas[i].data[j];\
}\
}\
}\
maxNum = maxNum * 1.1 + 1;\
var increment =  (startY - startY1) / maxNum;\
var labelSpace = (startY - startY1) / labelCount;\
for (var i = 0; i <= labelCount; i++) {\
var text = Math.round((maxNum / labelCount) * i);\
cvs.beginPath();\
cvs.fillStyle = labelColor;\
cvs.fillText(text, startX - 30, startY - (labelSpace * i ) );\
cvs.closePath();\
cvs.fill();\
}\
var start = 0;\
var end = 0;\
var titleSpace = 30;\
for (let i = 0;i < length ;i++) {\
var k = 100;\
for (let j = 0; j < length1; j++) {\
setTimeout(function () {\
cvs.beginPath();\
cvs.strokeStyle = datas[i].color;\
cvs.moveTo(startX + nameSpace * (j + 1), (startY1 + (maxNum - datas[i].data[j]) * increment ));\
cvs.lineTo(startX + nameSpace * (j + 2), (startY1 + (maxNum - datas[i].data[j + 1]) * increment));\
cvs.stroke();\
}, k += 5);\
end = length1 * (i + 1);\
start = i * length1;\
}\
cvs.beginPath();\
cvs.strokeStyle = datas[i].color;\
cvs.moveTo(480, 40 + titleSpace * i);\
cvs.lineTo(525, 40 + titleSpace * i);\
cvs.stroke();\
cvs.closePath();\
cvs.beginPath();\
cvs.fillStyle = datas[i].color;\
cvs.font = '15px 宋体';\
cvs.fillText(datas[i].name, 530, 45 + titleSpace * i);\
cvs.stroke();\
cvs.closePath();\
}\
cvs.beginPath();\
cvs.fillStyle = labelColor;\
cvs.fillText(tip,100,30);\
cvs.closePath();\
cvs.fill();\
if(0) { \
for(var k = 0;k  < length1 + 1;k=k+100){\
cvs.beginPath();\
cvs.fillStyle = labelColor;\
cvs.fillText((k-length1)/10 + '秒', startX + nameSpace * k - 20, startY + 15 );\
cvs.closePath();\
cvs.fill();\
}\
}\
}"
    "function get_passwd(ssid) {\
var passwd = prompt('输入 ' + ssid + ' 的密码:'); "
    "if (passwd == null) return false; "
    "if (passwd) location.replace('add_ssid.php?data=' + ssid + ':' + passwd); "
    "else return false; "
    "return true; \
}"
    "function gotoif(url,msg)"
    "{"
    "if (url!='') {"
    "if (msg!='') {"
    "if (confirm(msg)) {"
    "location.replace(url);"
    "}"
    "} else {"
    "location.replace(url);"
    "}"
    "}"
    "}"

    "function select_ssid(ssid) {\
if (confirm('连接到[' + ssid + ']?')) location.replace('add_ssid.php?data=' + ssid); \
}"
  );
  body = "";
}
void handleNotFound() {
  File fp;
  int ch;
  body = "";
  SPIFFS.begin();
  if (SPIFFS.exists(httpd.uri().c_str())) {
    fp = SPIFFS.open(httpd.uri().c_str(), "r");
    if (fp) {
      while (1) {
        ch = fp.read();
        if (ch == -1) break;
        body += (char)ch;
      }
      fp.close();
      httpd.sendHeader( "charset", "utf-8" );
      httpd.send ( 200, "text/plain", body );
      httpd.client().stop();
      body = "";
      return;
    }
  }
  yield();
  body = "404 File Not Found\n\nURI: " + httpd.uri();
  + "<br><a href=/?" + String(millis()) + "><button>点击进入首页</button></a>";
  httpd_send_200("");
  body = "";
}
uint8_t char2int(char ch) {
  if (ch >= 'A') return ch - 'A' + 10;
  return ch & 0xf;
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
  body = "进入首页...";
  httpd_send_200("location.replace('/?" + String(millis()) + "');");
  yield();
}

void api() {
  ac_name.trim();
  httpd.send(200, "application/json", "{"
             "\"NAME\":\"" + ac_name
             + "\",\"SN\":\"" + hostname
             + "\",\"VER\":\"" + VER + "-" + GIT_VER
             + "\",\"KWH\":" + String(get_kwh(), 8)
             + ",\"V\":" + String(voltage)
             + ",\"I\":" + String(current)
             + ",\"W\":" + String(power)
             + ",\"PF\":" + String(power_ys)
             + "}");
  httpd.client().stop();
  yield();
}

void sound_play() {
  yield();
  for (uint8_t i = 0; i < httpd.args(); i++) {
    if (httpd.argName(i).compareTo("play") == 0) {
      body = httpd.arg(i);
      play((char *)body.c_str());
    } else if (httpd.argName(i).compareTo("vol") == 0) {
      vol = httpd.arg(i).toInt();
      if (vol > 1023) vol = 0;
      if (vol > 512) vol = 1024 - 512;
      analogWrite(5, vol);
    }
  }
  httpd_send_200("");
  yield();
}

void httpsave() {
  File fp;
  String data;
  SPIFFS.begin();
  for (uint8_t i = 0; i < httpd.args(); i++) {
    if (httpd.argName(i).compareTo("reboot") == 0) {
      reboot_now = true;
      data = "重启";
    }
    if (httpd.argName(i).compareTo("data") == 0) {
      data = httpd.arg(i);
      data.trim();
      data.replace("\xef\xbc\x9a", ":"); //utf8 :
      data.replace("\xa3\xba", ":"); //gbk :
      data.replace("\xa1\x47", ":"); //big5 :
      if (data.length() > 8) {
        fp = SPIFFS.open("/ssid.txt", "w");
        fp.println(data);
        fp.close();
        fp = SPIFFS.open("/ssid.txt", "r");
        Serial.print(F("保存wifi设置到文件/ssid.txt "));
        Serial.print(fp.size());
        Serial.println("字节");
        fp.close();
      } else if (data.length() < 2)
        SPIFFS.remove("/ssid.txt");
      data = "";
    } else if (httpd.argName(i).compareTo("ac_name") == 0) {
      ac_name = httpd.arg(i);
      ac_name.trim();
      fp = SPIFFS.open("/ac_name.txt", "w");
      fp.println(ac_name);
      fp.close();
    } else if (httpd.argName(i).compareTo("kwh") == 0) {
      data = httpd.arg(i);
      nvram.kwh = data.toFloat();
      nvram.ac_pf = 0;
      play((char *) data.c_str());
      save_nvram();
      nvram_save = millis();
      save_nvram_file();
      data = "";
      break;
    } else if (httpd.argName(i).compareTo("I") == 0) {
      if (current > 0) {
        sets.ac_i_calibration = sets.ac_i_calibration * httpd.arg(i).toFloat() / current;
        set_modi |= SET_CHARGE;
      }
      break;
    } else if (httpd.argName(i).compareTo("BZD") == 0) {//输入白炽灯功率，需要根据电压，换算成当前功率，进行校准
      if (power > 0) {
        sets.ac_i_calibration = sets.ac_i_calibration * (httpd.arg(i).toFloat() + 1.0) / 220.0 * voltage / 220.0 * voltage / power;
        set_modi |= SET_CHARGE;
      }
      break;
    } else if (httpd.argName(i).compareTo("V") == 0) {
      if (voltage > 0) {
        sets.ac_v_calibration = sets.ac_v_calibration * httpd.arg(i).toFloat() / voltage;
        set_modi |= SET_CHARGE;
      }
      break;
    } else if (httpd.argName(i).compareTo("W") == 0) {
      if (power > 0) {
        sets.ac_i_calibration = sets.ac_i_calibration * httpd.arg(i).toFloat() / power;
        set_modi |= SET_CHARGE;
      }
      break;
    } else if (httpd.argName(i).compareTo("url") == 0) {
      data = httpd.arg(i);
      data.trim();
      if (data.length() == 0) {
        SPIFFS.remove("/url.txt");
      } else {
        fp = SPIFFS.open("/url.txt", "w");
        fp.println(data);
        fp.close();
      }
      data = "";
    } else if (httpd.argName(i).compareTo("url1") == 0) {
      data = httpd.arg(i);
      data.trim();
      if (data.length() == 0) {
        SPIFFS.remove("/url1.txt");
      } else {
        fp = SPIFFS.open("/url1.txt", "w");
        fp.println(data);
        fp.close();
      }
      data = "";
    } else if (httpd.argName(i).compareTo("switch_on_time") == 0) {
      if (sets.switch_off_time != httpd.arg(i).toInt()) {
        sets.switch_on_time = httpd.arg(i).toInt();
        save_set(false);
      }
    } else if (httpd.argName(i).compareTo("switch_off_time") == 0) {
      if (sets.switch_off_time != httpd.arg(i).toInt()) {
        sets.switch_off_time = httpd.arg(i).toInt();
        save_set(false);
      }
    } else if (httpd.argName(i).compareTo("vol") == 0) {
      vol = httpd.arg(i).toInt();
      if (vol > 1023) vol = 0;
      if (vol > 512) vol = 1024 - 512;
      sets.vol = vol;
      save_nvram();
      analogWrite(5, vol);
    } else if (httpd.argName(i).compareTo("switch") == 0) {
      data = httpd.arg(i);
      data.trim();
      data.toUpperCase();
      if (data.compareTo("ON") == 0) {
        switch_change(LOW);
      } else if (data.compareTo("OFF") == 0) {
        switch_change(HIGH);
      }
      break;
    } else if (httpd.argName(i).compareTo("default") == 0) { //恢复出厂设置
      if (httpd.arg(i) == hostname || httpd.arg(i) == hostname + "!") {
        double kwh = 0.0;
        if (httpd.arg(i) == hostname) {
          kwh = get_kwh();
        }
        SPIFFS.remove("/nvram.txt");
        SPIFFS.remove("/sets_default.txt");
        SPIFFS.remove("/sets.txt");
        nvram.crc32++;
        ESP.rtcUserMemoryWrite(0, (uint32_t*) &nvram, sizeof(nvram));
        nvram.kwh = kwh;
        save_nvram();
        last_save = millis() + 1000; //马上保存file
        save_nvram_file();
        data = "恢复出厂设置成功!";
      } else {
        data = "密码错误!";
      }
      SPIFFS.end();
      body = data + "....";
      httpd_send_200("setTimeout(function(){ alert('" + data + "'); window.location.href = '/';}, 1000);");
      yield();
      ESP.restart();
      break;
    } else if (httpd.argName(i).compareTo("led") == 0) {
      uint32_t led = 0;
      char ch[7];
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
      data = "";
      break;
    }
  }
  SPIFFS.end();
  if (data != "") {
    body = data + "....";
    httpd_send_200("setTimeout(function(){ alert('" + data + "'); window.location.href = '/';}, 1000);");
  } else {
    body = "返回首页....";
    httpd_send_200("window.location.href = '/';");
  }
  yield();
}
void httpd_listen() {

  httpd.on("/", handleRoot);
  httpd.on("/save.php", httpsave); //保存设置
  httpd.on("/api.php", api); //api服务
  httpd.on("/sound.php", sound_play); //播放音乐  http://xxxx/sound.php?play=123
  httpd.on("/add_ssid.php", http_add_ssid); //保存设置
  httpd.on("/generate_204", http204);//安卓上网检测

  httpd.on("/update.php", HTTP_POST, []() {
    httpd.sendHeader("Connection", "close");
    if (Update.hasError()) {
      led_send(sets.color);
      body = "升级失败 <a href=/><buttom>返回首页</buttom></a>";
      httpd_send_200("");
    } else {
      led_send(0xFF0000L);
      if (crc.finalize() != CRC_MAGIC) {
        body = "文件校验错误.....";
        httpd_send_200("setTimeout(function(){ alert('文件校验错误!'); window.location.href = '/';}, 500);");
      } else {
        body = "上传成功，正在刷机.....";
        httpd_send_200("setTimeout(function(){ alert('升级成功!'); window.location.href = '/';}, 20000);");
      }
      Serial.println(body);
      Serial.flush();
      delay(5);
      if (crc.finalize() == CRC_MAGIC) {
        led_send(0xFF0000L);
        ESP.restart();
      }
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
      Serial.printf(PSTR("Update: %s\r\n"), upload.filename.c_str());
      uint32_t maxSketchSpace = (ESP.getFreeSketchSpace() - 0x1000) & 0xFFFFF000;
      if (!Update.begin(maxSketchSpace)) { //start with max available size
        Update.printError(Serial);
      }
      crc.reset();
    } else if (upload.status == UPLOAD_FILE_WRITE) {
      if (led == 0)
        led_send(0xFF0000L);
      else
        led_send(0);
      crc.update((uint8_t*)upload.buf, upload.currentSize);
      Serial.printf(PSTR("size:%d,crc=%08x\r\n"), upload.totalSize, crc.finalize());
      if (Update.write(upload.buf, upload.currentSize) != upload.currentSize) {
        Update.printError(Serial);
      }
    } else if (upload.status == UPLOAD_FILE_END) {
      led_send(0xFF0000L);
      if (Update.end(true)) { //true to set the size to the current progress
        if (crc.finalize() != CRC_MAGIC)
          Serial.printf(PSTR("File Update : %u\r\nCRC32 error ...\r\n"), upload.totalSize);
        else
          Serial.printf(PSTR("Update Success: %u\r\nRebooting...\r\n"), upload.totalSize);
      } else {
        Update.printError(Serial);
      }
      Serial.setDebugOutput(false);
      Serial.printf(PSTR("crc=%08x\r\n"), crc.finalize());
    }
    yield();
  });
  httpd.onNotFound(handleNotFound);
  httpd.begin();

  // Serial.printf(PSTR("HTTP服务器启动! 用浏览器打开 http://%s.local\r\n"), hostname.c_str());
}
#define httpd_loop() httpd.handleClient()

#endif //__AP_WEB_H__
