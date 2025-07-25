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
    if(netlog.connected()) {
      netlog.println("new client come in, then you will be offline.");
      netlog.stop();
   }
   netlog = tcpServer.available();
   netlog.println("welcome in.");
  }
}

void loga(String msg) {
  if (netlog.availableForWrite())
    netlog.println(msg);
}

#endif //__NETLOG_H__

