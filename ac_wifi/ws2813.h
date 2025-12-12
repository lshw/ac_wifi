#ifndef __WS2813_H__
#define __WS2813_H__
#define LEDP 13                                        /* 接ws2813的 gpio */
#define _300ns ((uint32_t)F_CPU / (1000000000L / 300)) /*300n需要的时钟周期数 */
#define _780ns ((uint32_t)F_CPU / (1000000000L / 780))
#define _1000ns ((uint32_t)F_CPU / (1000000000L / 1000))
uint32_t pinMask = _BV(LEDP);
static uint32_t CycleCount(void) __attribute__((always_inline));
static inline uint32_t CycleCount(void) {
  uint32_t ccount;
  __asm__ __volatile__("rsr %0,ccount"
                       : "=a"(ccount));
  return ccount;
}
uint32_t led = 0;
IRAM_ATTR void led_send(uint32_t dat) {
  //dat:0xc0d0e0  => html color #C0D0E0
  //红绿蓝 -> 绿红蓝
  if (led == dat) return;
  led = dat;
  //    if (sets.serial > 2) { //序号小于3的产品使用的是RGB顺序的led，其它是GRB
  dat &= 0xffL;
  dat |= ((led << 8) & 0xff0000L);
  dat |= ((led >> 8) & 0xff00L);
  dat &= 0xffffffL;
  //   }
  uint32_t mask = 0x800000;
  uint32_t t0, t1;
  cli();
  while (mask) {
    if (dat & mask) {
      t1 = _780ns;
    } else {
      t1 = _300ns;
    }
    t0 = CycleCount();                               //当前的cpu时钟数
    GPIO_REG_WRITE(GPIO_OUT_W1TS_ADDRESS, pinMask);  // 置高
    while (((CycleCount()) - t0) < t1)
      ;  // 发送T0H or T1H

    GPIO_REG_WRITE(GPIO_OUT_W1TC_ADDRESS, pinMask);  // 置低
    while ((CycleCount() - t0) < _1000ns)
      ;          // 等待发送周期1.2us
    mask >>= 1;  // 发送下一位
  }
  sei();
}
#endif  //__ws2813_h__
