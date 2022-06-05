struct calibration {
  uint32_t serial;
  float i;
  float v;
} __attribute__ ((packed)) calibrations[] = {
  {0L, 1.44 / (0.0019 * 1000), 1.881}, //default
  {0x1889C2L, 1.44 / (0.0019 * 1000), 1.881}, //serial:1
  {0x1889D3L, 1.44 / (0.0019 * 1000), 1.881}, //serial:2
  {0x1888D6L, 1.44 / (0.0019 * 1000), 1.881}, //serial:3
  {0x191941L, 1.44 / (0.0019 * 1000), 1.881}, //serial:4
  {0x19196DL, 1.44 / (0.0019 * 1000), 1.881}, //serial:5
  {0x521E6CL, 1.44 / (0.0019 * 1000), 1.881}, //serial:6
};
