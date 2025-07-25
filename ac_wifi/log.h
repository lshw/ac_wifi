#ifndef __LOG_H__
#define __LOG_H__

#include <ESP8266WebServer.h>
WiFiServer tcpServer(23);
WiFiClient tcpClient;

void log_setup() {
  tcpServer.begin();
  tcpServer.setNoDelay(true);
}

void log_loop() {
  if (tcpServer.hasClient()) {
    if(tcpClient.connected()) {
      tcpClient.println("new client come in, then you will be offline.");
      tcpClient.stop();
   }
   tcpClient = tcpServer.available();
   tcpClient.println("welcome in.");
  }
}

void log(String msg) {
  if (tcpClient.availableForWrite())
    tcpClient.println(msg);
}

#endif //__LOG_H__

