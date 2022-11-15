struct calibration {
  uint32_t serial;
  float i; //电流校准
  float v; //电压校准
  uint8_t i_max; //限流 电流值
} __attribute__ ((packed)) calibrations[] = {
  {0, 1.44 / (0.0019 * 1000), 1.88, 0}, //default
  {0x1889C9, 0.709966 / 200 * 201, 1.860806, 0}, //1
  {0x53039D, 0.551934 / 200 * 201, 1.890511, 0}, //2
  {0x51C92A, 0.707815 / 200 * 201, 1.8e68270, 0}, //3
  {0x4505B4, 0.738579 / 200 * 201, 1.878223, 0}, //4
  {0x460D73, 0.792258 / 200 * 201, 1.868901, 0}, //5
  {0x52A2CC, 0.707864 / 200 * 201, 1.867087, 0}, //6
  {0x44ECAB, 0.738591 / 200 * 201, 1.86435, 0}, //7
  {0x45B184, 0.74965 / 200 * 201, 1.875595, 0}, //8
  {0x44E0D1, 0.605586 / 200 * 201, 1.867409, 0}, //9
  {0x521E6C, 0.679462 / 200 * 201, 1.864274, 0}, //10
  //new pcb
  {0x19196D, 0.611581 / 200 * 201, 1.874211, 10}, //11
  {0x1889C2, 0.574081 / 200 * 201, 1.873244, 10}, //12
  {0x191941, 0.548322 / 200 * 201, 1.866311, 10}, //13
  {0x1889D3, 0.633888 / 200 * 201, 1.870392, 10}, //14
  {0x1888D6, 0.693933 / 200 * 201, 1.866092, 10}, //15
  {0x53048E, 0.483820 / 200 * 201, 1.875600, 10}, //16
  {0x52E879, 0.612531 / 200 * 201, 1.870688, 10}, //17
  {0x530314, 0.676752 / 200 * 201, 1.872516, 10}, //18
  {0x5217AB, 1.44 / (0.0019 * 1000), 1.881, 0}, //
  {0x52E940, 0.551763 / 200 * 201, 1.873173, 10}, //20
  {0x52E865, 0.723618 / 200 * 201, 1.868786, 16}, //21 16A
  {0x52E928, 0.820342 / 200 * 201, 1.874871, 16}, //22  16A
  {0x23, 0.7, 1.88, 16}, //23 16A
  {0x24, 0.7, 1.88, 16}, //24 16A
  {0x25, 0.7, 1.88, 16}, //25 16A
  {0x26, 0.7, 1.88, 16}, //26 16A
  {0x27, 0.7, 1.88, 16}, //27 16A
  {0x28, 0.7, 1.88, 16}, //28 16A
  {0x29, 0.7, 1.88, 16}, //29 16A
  {0x30, 0.7, 1.88, 16}, //30 16A
  {0x53044F, 0.695653 / 200 * 201, 1.874642, 10}, //31 10A
  {0x5303A5, 0.790019 / 200 * 201, 1.883172, 10}, //32 10A
  {0x52E7F2, 0.775669 / 200 * 201, 1.872131, 10}, //33 10A
  {0x53034E, 0.739899 / 200 * 201, 1.882401, 10}, //34 10A
  {0x52E930, 0.858341 / 200 * 201, 1.873929, 10}, //35 10A
  {0x530205, 0.728980 / 200 * 201, 1.874243, 10}, //36 10A
  {0x52E9CC, 0.7, 1.88, 10}, //37 10A
  {0x52FF87, 0.7, 1.88, 10}, //38 10A
  {0x39, 0.7, 1.88, 10}, //39 10A
  {0x40, 0.7, 1.88, 10}, //40 10A
};
