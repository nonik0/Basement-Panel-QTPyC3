// ChatGPT couldn't cope so fixed up with https://tchapi.github.io/Adafruit-GFX-Font-Customiser/

#include <Adafruit_GFX.h>

// Bitmap data for the 3x4 font
const uint8_t Font3x4Bitmaps[] PROGMEM = {
  0x00, 0x00, 0xD0, 0xA0, 0x2E, 0x74, 0x79, 0xE0, 0xA9, 0x50, 0x53, 0x50, 
  0xC0, 0x69, 0x96, 0xDD, 0x80, 0x5D, 0x00, 0x60, 0xC0, 0xE0, 0x29, 0x40, 
  0xF6, 0xF0, 0x59, 0x70, 0xE7, 0x70, 0xEC, 0xF0, 0xB7, 0x90, 0xF8, 0xF0, 
  0x9E, 0xF0, 0xE5, 0x20, 0xFE, 0xF0, 0xF7, 0x90, 0xA0, 0xA0, 0x71, 0x80, 
  0xE3, 0x80, 0xC7, 0x00, 0xE5, 0x20, 0xFE, 0x70, 0x57, 0xD0, 0xDE, 0xF0, 
  0xF2, 0x70, 0xD6, 0xE0, 0xFA, 0x70, 0xF3, 0x40, 0xF2, 0x70, 0xBE, 0xD0, 
  0xE9, 0x70, 0x26, 0xF0, 0xBB, 0xD0, 0x92, 0x70, 0xFE, 0xD0, 0xD6, 0xD0, 
  0xF6, 0xF0, 0xF7, 0xC0, 0xF6, 0xE0, 0xF7, 0x50, 0xF1, 0xF0, 0xE9, 0x20, 
  0xB6, 0xF0, 0xB6, 0xA0, 0xB7, 0xF0, 0xAA, 0xD0, 0xB5, 0x20, 0xE7, 0x70, 
  0xEB, 0x89, 0x10, 0xD7, 0x56, 0x80, 0xE0, 0x90, 0x0E, 0xF0, 0x9A, 0xF0, 
  0x0E, 0xF0, 0x2E, 0xF0, 0x1F, 0x70, 0x73, 0xC0, 0x1F, 0xB0, 0x9A, 0xD0, 
  0x8B, 0x20, 0xE0, 0x97, 0x50, 0xF0, 0x1F, 0xD0, 0x1A, 0xD0, 0x1E, 0xF0, 
  0x1F, 0xC0, 0x1F, 0x90, 0x0E, 0x40, 0x0D, 0x60, 0x5D, 0x30, 0x16, 0xF0, 
  0x16, 0xA0, 0x17, 0xF0, 0x15, 0x50, 0x15, 0x20, 0x19, 0x30, 0x6B, 0x30, 
  0xF0, 0xC9, 0xE0, 0x78, 0x00
};

const GFXglyph Font3x4Glyphs[] PROGMEM = {
  {     0,   3,   4,   4,    0,   -3 },   // 0x20 ' '
  {     2,   1,   4,   2,    0,   -3 },   // 0x21 '!'
  {     3,   3,   1,   4,    0,   -3 },   // 0x22 '"'
  {     4,   4,   4,   5,    0,   -3 },   // 0x23 '#'
  {     6,   3,   4,   4,    0,   -3 },   // 0x24 '$'
  {     8,   3,   4,   4,    0,   -3 },   // 0x25 '%'
  {    10,   3,   4,   4,    0,   -3 },   // 0x26 '&'
  {    12,   1,   2,   2,    0,   -3 },   // 0x27 '''
  {    13,   2,   4,   3,    0,   -3 },   // 0x28 '('
  {    14,   2,   4,   3,    0,   -3 },   // 0x29 ')'
  {    15,   3,   3,   4,    0,   -3 },   // 0x2A '*'
  {    17,   3,   3,   4,    0,   -3 },   // 0x2B '+'
  {    19,   2,   2,   3,    0,   -1 },   // 0x2C ','
  {    20,   2,   1,   4,    1,   -2 },   // 0x2D '-'
  {    21,   1,   1,   4,    1,   -1 },   // 0x2E '.'
  {    22,   3,   4,   4,    0,   -3 },   // 0x2F '/'
  {    24,   3,   4,   4,    0,   -3 },   // 0x30 '0'
  {    26,   3,   4,   4,    0,   -3 },   // 0x31 '1'
  {    28,   3,   4,   4,    0,   -3 },   // 0x32 '2'
  {    30,   3,   4,   4,    0,   -3 },   // 0x33 '3'
  {    32,   3,   4,   4,    0,   -3 },   // 0x34 '4'
  {    34,   3,   4,   4,    0,   -3 },   // 0x35 '5'
  {    36,   3,   4,   4,    0,   -3 },   // 0x36 '6'
  {    38,   3,   4,   4,    0,   -3 },   // 0x37 '7'
  {    40,   3,   4,   4,    0,   -3 },   // 0x38 '8'
  {    42,   3,   4,   4,    0,   -3 },   // 0x39 '9'
  {    44,   1,   4,   2,    0,   -3 },   // 0x3A ':'
  {    45,   1,   4,   2,    0,   -3 },   // 0x3B ';'
  {    46,   3,   3,   4,    0,   -3 },   // 0x3C '<'
  {    48,   3,   3,   4,    0,   -3 },   // 0x3D '='
  {    50,   3,   3,   4,    0,   -3 },   // 0x3E '>'
  {    52,   3,   4,   4,    0,   -3 },   // 0x3F '?'
  {    54,   3,   4,   4,    0,   -3 },   // 0x40 '@'
  {    56,   3,   4,   4,    0,   -3 },   // 0x41 'A'
  {    58,   3,   4,   4,    0,   -3 },   // 0x42 'B'
  {    60,   3,   4,   4,    0,   -3 },   // 0x43 'C'
  {    62,   3,   4,   4,    0,   -3 },   // 0x44 'D'
  {    64,   3,   4,   4,    0,   -3 },   // 0x45 'E'
  {    66,   3,   4,   4,    0,   -3 },   // 0x46 'F'
  {    68,   3,   4,   4,    0,   -3 },   // 0x47 'G'
  {    70,   3,   4,   4,    0,   -3 },   // 0x48 'H'
  {    72,   3,   4,   4,    0,   -3 },   // 0x49 'I'
  {    74,   3,   4,   4,    0,   -3 },   // 0x4A 'J'
  {    76,   3,   4,   4,    0,   -3 },   // 0x4B 'K'
  {    78,   3,   4,   4,    0,   -3 },   // 0x4C 'L'
  {    80,   3,   4,   4,    0,   -3 },   // 0x4D 'M'
  {    82,   3,   4,   4,    0,   -3 },   // 0x4E 'N'
  {    84,   3,   4,   4,    0,   -3 },   // 0x4F 'O'
  {    86,   3,   4,   4,    0,   -3 },   // 0x50 'P'
  {    88,   3,   4,   4,    0,   -3 },   // 0x51 'Q'
  {    90,   3,   4,   4,    0,   -3 },   // 0x52 'R'
  {    92,   3,   4,   4,    0,   -3 },   // 0x53 'S'
  {    94,   3,   4,   4,    0,   -3 },   // 0x54 'T'
  {    96,   3,   4,   4,    0,   -3 },   // 0x55 'U'
  {    98,   3,   4,   4,    0,   -3 },   // 0x56 'V'
  {   100,   3,   4,   4,    0,   -3 },   // 0x57 'W'
  {   102,   3,   4,   4,    0,   -3 },   // 0x58 'X'
  {   104,   3,   4,   4,    0,   -3 },   // 0x59 'Y'
  {   106,   3,   4,   4,    0,   -3 },   // 0x5A 'Z'
  {   108,   2,   4,   4,    0,   -3 },   // 0x5B '['
  {   109,   3,   4,   4,    0,   -3 },   // 0x5C '\'
  {   111,   2,   4,   4,    0,   -3 },   // 0x5D ']'
  {   112,   3,   3,   4,    0,   -3 },   // 0x5E '^'
  {   114,   3,   1,   4,    0,   -1 },   // 0x5F '_'
  {   115,   2,   2,   3,    0,   -3 },   // 0x60 '`'
  {   116,   3,   4,   4,    0,   -3 },   // 0x61 'a'
  {   118,   3,   4,   4,    0,   -3 },   // 0x62 'b'
  {   120,   3,   4,   4,    0,   -3 },   // 0x63 'c'
  {   122,   3,   4,   4,    0,   -3 },   // 0x64 'd'
  {   124,   3,   4,   4,    0,   -3 },   // 0x65 'e'
  {   126,   3,   4,   4,    0,   -3 },   // 0x66 'f'
  {   128,   3,   4,   4,    0,   -3 },   // 0x67 'g'
  {   130,   3,   4,   4,    0,   -3 },   // 0x68 'h'
  {   132,   2,   4,   4,    1,   -3 },   // 0x69 'i'
  {   133,   3,   4,   4,    0,   -3 },   // 0x6A 'j'
  {   135,   3,   4,   4,    0,   -3 },   // 0x6B 'k'
  {   137,   1,   4,   2,    0,   -3 },   // 0x6C 'l'
  {   138,   3,   4,   4,    0,   -3 },   // 0x6D 'm'
  {   140,   3,   4,   4,    0,   -3 },   // 0x6E 'n'
  {   142,   3,   4,   4,    0,   -3 },   // 0x6F 'o'
  {   144,   3,   4,   4,    0,   -3 },   // 0x70 'p'
  {   146,   3,   4,   4,    0,   -3 },   // 0x71 'q'
  {   148,   3,   4,   4,    0,   -3 },   // 0x72 'r'
  {   150,   3,   4,   4,    0,   -3 },   // 0x73 's'
  {   152,   3,   4,   4,    0,   -3 },   // 0x74 't'
  {   154,   3,   4,   4,    0,   -3 },   // 0x75 'u'
  {   156,   3,   4,   4,    0,   -3 },   // 0x76 'v'
  {   158,   3,   4,   4,    0,   -3 },   // 0x77 'w'
  {   160,   3,   4,   4,    0,   -3 },   // 0x78 'x'
  {   162,   3,   4,   4,    0,   -3 },   // 0x79 'y'
  {   164,   3,   4,   4,    0,   -3 },   // 0x7A 'z'
  {   166,   3,   4,   4,    0,   -3 },   // 0x7B '{'
  {   168,   1,   4,   2,    0,   -3 },   // 0x7C '|'
  {   169,   3,   4,   4,    0,   -3 },   // 0x7D '}'
  {   171,   3,   2,   4,    0,   -2 }    // 0x7E '~'
};

const GFXfont Font3x4 PROGMEM = {
    (uint8_t *)Font3x4Bitmaps,     // Pointer to the bitmap data
    (GFXglyph *)Font3x4Glyphs, 0x20, 0x7E,                           // Last ASCII character ('~')
    4                           // Font height in pixels
};
