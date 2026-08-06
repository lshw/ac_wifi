#ifndef __NETLOG_H__
#define __NETLOG_H__

#include <ESP8266WebServer.h>
WiFiServer tcpServer(23);
WiFiClient netlog;

void netlog_setup() {
  tcpServer.begin();
  tcpServer.setNoDelay(true);
}

void netlog_loop() {
  if (tcpServer.hasClient()) {
    if (netlog.connected()) {
      set0.console->println(
          "\r\nnew client come in, then you will be offline.");
      netlog.stop();
    }
    netlog = tcpServer.available();
    set0.console = &netlog;
    set0.console->println("\r\nwelcome in.");
  }
  if (!netlog.connected()) {
    set0.console = &Serial;
  }
}

#endif //__NETLOG_H__
