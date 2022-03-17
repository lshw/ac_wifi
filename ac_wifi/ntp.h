#ifdef __NTP_H__
#define __NTP_H__

#include <Udp.h>
WiFiUDP ntpUDP;
#define NTP_PACKET_SIZE 48
uint32_t ntp_get(const char * ServerName) {
  byte  buff[NTP_PACKET_SIZE];
  memset(buff, 0, sizeof(buff));
  buff[0] = 0b11100011;   // LI, Version, Mode
  buff[1] = 0;     // Stratum, or type of clock
  buff[2] = 6;     // Polling Interval
  buff[3] = 0xEC;  // Peer Clock Precision
  // 8 bytes of zero for Root Delay & Root Dispersion
  buff[12]  = 'I';
  buff[13]  = 'N';
  buff[14]  = 'I';
  buff[15]  = 'R';

  ntpUDP->beginPacket(ServerName, 123); //NTP requests are to port 123
  ntpUDP->write(buff, NTP_PACKET_SIZE);
  ntpUDP->endPacket();

  byte timeout = 0;
  int cb = 0;
  do {
    delay ( 10 );
    cb = ntpUDP->parsePacket();
    if (timeout > 100) return false; // timeout after 1000 ms
    timeout++;
  } while (cb == 0);

  ntpUDP->read(buff, NTP_PACKET_SIZE);

  return (uint32_t) (buff[40] << 24) | (buff[41] << 16) | (buff[42] << 8) | buff[43];
}

#endif //__NTP_H__
