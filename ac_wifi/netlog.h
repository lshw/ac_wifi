#ifndef __NETLOG_H__
#define __NETLOG_H__

#include <ESP8266WebServer.h>
WiFiServer tcpServer(23);
WiFiClient netlog;
#define LOG(format, ...) \
  if (netlog.connected()) netlog.printf(PSTR(format), ##__VA_ARGS__); \
  if (Serial) Serial.printf(PSTR(format), ##__VA_ARGS__);

void netlog_setup() {
  tcpServer.begin();
  tcpServer.setNoDelay(true);
}

void netlog_loop() {
  if (tcpServer.hasClient()) {
    if (netlog.connected()) {
      LOG("\r\nnew client come in, then you will be offline.\r\n");
      netlog.stop();
    }
    netlog = tcpServer.available();
    LOG("\r\nwelcome in.\r\n");
  }
}

#endif  //__NETLOG_H__
