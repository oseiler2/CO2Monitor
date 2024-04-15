// '0', 8x10px
const unsigned char digits_0[] PROGMEM = {
  0x3c, 0x7e, 0x66, 0x66, 0x66, 0x66, 0x66, 0x66, 0x7e, 0x3c
};
// '1', 8x10px
const unsigned char digits_1[] PROGMEM = {
  0x18, 0x18, 0x18, 0x18, 0x18, 0x18, 0x18, 0x18, 0x18, 0x18
};
// '2', 8x10px
const unsigned char digits_2[] PROGMEM = {
  0x3c, 0x7e, 0x66, 0x06, 0x0e, 0x1c, 0x38, 0x70, 0x7e, 0x7e
};
// '3', 8x10px
const unsigned char digits_3[] PROGMEM = {
  0x3c, 0x7e, 0x66, 0x06, 0x1e, 0x1e, 0x06, 0x66, 0x7e, 0x3c
};
// '4', 8x10px
const unsigned char digits_4[] PROGMEM = {
  0x66, 0x66, 0x66, 0x66, 0x7e, 0x7e, 0x06, 0x06, 0x06, 0x06
};
// '5', 8x10px
const unsigned char digits_5[] PROGMEM = {
  0x7e, 0x7e, 0x60, 0x7c, 0x7e, 0x06, 0x06, 0x66, 0x7e, 0x3c
};
// '6', 8x10px
const unsigned char digits_6[] PROGMEM = {
  0x3c, 0x7e, 0x66, 0x60, 0x7c, 0x7e, 0x66, 0x66, 0x7e, 0x3c
};
// '7', 8x10px
const unsigned char digits_7[] PROGMEM = {
  0x7e, 0x7e, 0x06, 0x06, 0x06, 0x06, 0x06, 0x06, 0x06, 0x06
};
// '8', 8x10px
const unsigned char digits_8[] PROGMEM = {
  0x3c, 0x7e, 0x66, 0x66, 0x3c, 0x7e, 0x66, 0x66, 0x7e, 0x3c
};
// '9', 8x10px
const unsigned char digits_9[] PROGMEM = {
  0x3c, 0x7e, 0x66, 0x66, 0x7e, 0x3e, 0x06, 0x66, 0x7e, 0x3c
};

// '0_2', 16x20px
const unsigned char digits_0_16[] PROGMEM = {
  0x0f, 0xf0, 0x0f, 0xf0, 0x3f, 0xfc, 0x3f, 0xfc, 0x3c, 0x3c, 0x3c, 0x3c, 0x3c, 0x3c, 0x3c, 0x3c,
  0x3c, 0x3c, 0x3c, 0x3c, 0x3c, 0x3c, 0x3c, 0x3c, 0x3c, 0x3c, 0x3c, 0x3c, 0x3c, 0x3c, 0x3c, 0x3c,
  0x3f, 0xfc, 0x3f, 0xfc, 0x0f, 0xf0, 0x0f, 0xf0
};
// '1_2', 16x20px
const unsigned char digits_1_16[] PROGMEM = {
  0x03, 0xc0, 0x03, 0xc0, 0x03, 0xc0, 0x03, 0xc0, 0x03, 0xc0, 0x03, 0xc0, 0x03, 0xc0, 0x03, 0xc0,
  0x03, 0xc0, 0x03, 0xc0, 0x03, 0xc0, 0x03, 0xc0, 0x03, 0xc0, 0x03, 0xc0, 0x03, 0xc0, 0x03, 0xc0,
  0x03, 0xc0, 0x03, 0xc0, 0x03, 0xc0, 0x03, 0xc0
};
// '2_2', 16x20px
const unsigned char digits_2_16[] PROGMEM = {
  0x0f, 0xf0, 0x0f, 0xf0, 0x3f, 0xfc, 0x3f, 0xfc, 0x3c, 0x3c, 0x3c, 0x3c, 0x00, 0x3c, 0x00, 0x3c,
  0x00, 0xfc, 0x00, 0xfc, 0x03, 0xf0, 0x03, 0xf0, 0x0f, 0xc0, 0x0f, 0xc0, 0x3f, 0x00, 0x3f, 0x00,
  0x3f, 0xfc, 0x3f, 0xfc, 0x3f, 0xfc, 0x3f, 0xfc
};
// '3_2', 16x20px
const unsigned char digits_3_16[] PROGMEM = {
  0x0f, 0xf0, 0x0f, 0xf0, 0x3f, 0xfc, 0x3f, 0xfc, 0x3c, 0x3c, 0x3c, 0x3c, 0x00, 0x3c, 0x00, 0x3c,
  0x03, 0xfc, 0x03, 0xfc, 0x03, 0xfc, 0x03, 0xfc, 0x00, 0x3c, 0x00, 0x3c, 0x3c, 0x3c, 0x3c, 0x3c,
  0x3f, 0xfc, 0x3f, 0xfc, 0x0f, 0xf0, 0x0f, 0xf0
};
// '4_2', 16x20px
const unsigned char digits_4_16[] PROGMEM = {
  0x3c, 0x3c, 0x3c, 0x3c, 0x3c, 0x3c, 0x3c, 0x3c, 0x3c, 0x3c, 0x3c, 0x3c, 0x3c, 0x3c, 0x3c, 0x3c,
  0x3f, 0xfc, 0x3f, 0xfc, 0x3f, 0xfc, 0x3f, 0xfc, 0x00, 0x3c, 0x00, 0x3c, 0x00, 0x3c, 0x00, 0x3c,
  0x00, 0x3c, 0x00, 0x3c, 0x00, 0x3c, 0x00, 0x3c
};
// '5_2', 16x20px
const unsigned char digits_5_16[] PROGMEM = {
  0x3f, 0xfc, 0x3f, 0xfc, 0x3f, 0xfc, 0x3f, 0xfc, 0x3c, 0x00, 0x3c, 0x00, 0x3f, 0xf0, 0x3f, 0xf0,
  0x3f, 0xfc, 0x3f, 0xfc, 0x00, 0x3c, 0x00, 0x3c, 0x00, 0x3c, 0x00, 0x3c, 0x3c, 0x3c, 0x3c, 0x3c,
  0x3f, 0xfc, 0x3f, 0xfc, 0x0f, 0xf0, 0x0f, 0xf0
};
// '6_2', 16x20px
const unsigned char digits_6_16[] PROGMEM = {
  0x0f, 0xf0, 0x0f, 0xf0, 0x3f, 0xfc, 0x3f, 0xfc, 0x3c, 0x3c, 0x3c, 0x3c, 0x3c, 0x00, 0x3c, 0x00,
  0x3f, 0xf0, 0x3f, 0xf0, 0x3f, 0xfc, 0x3f, 0xfc, 0x3c, 0x3c, 0x3c, 0x3c, 0x3c, 0x3c, 0x3c, 0x3c,
  0x3f, 0xfc, 0x3f, 0xfc, 0x0f, 0xf0, 0x0f, 0xf0
};
// '7_2', 16x20px
const unsigned char digits_7_16[] PROGMEM = {
  0x3f, 0xfc, 0x3f, 0xfc, 0x3f, 0xfc, 0x3f, 0xfc, 0x00, 0x3c, 0x00, 0x3c, 0x00, 0x3c, 0x00, 0x3c,
  0x00, 0x3c, 0x00, 0x3c, 0x00, 0x3c, 0x00, 0x3c, 0x00, 0x3c, 0x00, 0x3c, 0x00, 0x3c, 0x00, 0x3c,
  0x00, 0x3c, 0x00, 0x3c, 0x00, 0x3c, 0x00, 0x3c
};
// '8_2', 16x20px
const unsigned char digits_8_16[] PROGMEM = {
  0x0f, 0xf0, 0x0f, 0xf0, 0x3f, 0xfc, 0x3f, 0xfc, 0x3c, 0x3c, 0x3c, 0x3c, 0x3c, 0x3c, 0x3c, 0x3c,
  0x0f, 0xf0, 0x0f, 0xf0, 0x3f, 0xfc, 0x3f, 0xfc, 0x3c, 0x3c, 0x3c, 0x3c, 0x3c, 0x3c, 0x3c, 0x3c,
  0x3f, 0xfc, 0x3f, 0xfc, 0x0f, 0xf0, 0x0f, 0xf0
};
// '9_2', 16x20px
const unsigned char digits_9_16[] PROGMEM = {
  0x0f, 0xf0, 0x0f, 0xf0, 0x3f, 0xfc, 0x3f, 0xfc, 0x3c, 0x3c, 0x3c, 0x3c, 0x3c, 0x3c, 0x3c, 0x3c,
  0x3f, 0xfc, 0x3f, 0xfc, 0x0f, 0xfc, 0x0f, 0xfc, 0x00, 0x3c, 0x00, 0x3c, 0x3c, 0x3c, 0x3c, 0x3c,
  0x3f, 0xfc, 0x3f, 0xfc, 0x0f, 0xf0, 0x0f, 0xf0
};
// Array of all bitmaps for convenience. (Total bytes used to store images in PROGMEM = 320)
const unsigned char* digits[10] = {
  digits_0,
  digits_1,
  digits_2,
  digits_3,
  digits_4,
  digits_5,
  digits_6,
  digits_7,
  digits_8,
  digits_9
};

// Array of all bitmaps for convenience. (Total bytes used to store images in PROGMEM = 320)
const unsigned char* digits_16[10] = {
  digits_0_16,
  digits_1_16,
  digits_2_16,
  digits_3_16,
  digits_4_16,
  digits_5_16,
  digits_6_16,
  digits_7_16,
  digits_8_16,
  digits_9_16
};