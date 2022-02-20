
#include <FS.h>
extern "C" {
#include "user_interface.h"
}
#define LEDP 13
uint32_t t300=300;
uint32_t t800=800;
void _400n(){
  for(uint32_t i = 0; i< 400; i ++);
}
void _450n(){
  for(uint32_t i = 0; i< 450; i ++);
}
void _850n(){
  for(uint32_t i = 0; i< 850; i ++);
}
void _800n(){
  for(uint32_t i = 0; i< 800; i ++);
}
uint8_t test=0;
void send(uint32_t dat) {
  cli();
    for(uint8_t i = 0; i < 24; i++) {
      digitalWrite(LEDP,HIGH);
      if(dat & 0x800000L == 0){
        _400n();
      digitalWrite(LEDP,LOW);
        _850n();
      }else{
        _800n();
      digitalWrite(LEDP,LOW);
        _450n();
      }
      dat = dat << 1;
    }
  sei();
}
void sound(uint8_t range,uint16_t len) {
  uint16_t snd[8]={0,523,587,659,698,784,880,980};
  analogWriteFreq(snd[range]);
  delay(len);
}
void sound(uint8_t range) {
  sound(range,300);
}
void play(char * qz) {
  uint16_t i0=strlen(qz);
  for(uint16_t i=0; i<i0; i++) sound(qz[i] & 0xf);
}
void setup(){
  pinMode(LEDP,OUTPUT);
  Serial.begin(115200);
  Serial.println("hello");
  uint32_t ms0=millis();
/*  for(uint8_t i0=0;i0<100;i0++) {
   send(0x0f0000L);//3.6ms
}
     Serial.printf("360ms=%d\r\n",millis()-ms0); 

*/
  delay(300);
  //analogWrite(5,50);
  //play("113477431131");
  analogWrite(5,0);
}
void loop(){
   system_soft_wdt_feed ();
  delay(1);
  send(0xff0000L);
  send(0xff0000L);
  delay(500);
  send(0xff00L);
  send(0xff00L);
  delay(500);
  send(0xffL);
  send(0xffL);
  delay(500);

}
