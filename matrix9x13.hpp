#include <Adafruit_IS31FL3741.h>

#include "Font3x4.h"
#include <Fonts/TomThumb.h>

extern volatile bool display;
TaskHandle_t matrix9x13TaskHandle = NULL;

Adafruit_IS31FL3741_QT_buffered matrix9x13;
char *matrix9x13Message;
char *matrix9x13Message2;
int text_x = matrix9x13.width(); // Initial text position = off right edge
int text_x2 = matrix9x13.width(); // Initial text position = off right edge
int text_y = 3;
int text_y2 = 8;
int text_min; // Pos. where text resets (calc'd later)
int text_min2;
uint16_t hue_offset = 0;

void Matrix9x13Task(void *parameters);

void Matrix9x13Setup()
{
  matrix9x13Message = new char[100];
  matrix9x13Message2 = new char[100];
  strcpy(matrix9x13Message, "Stella is bella!");
  strcpy(matrix9x13Message2, "Beau fo sho!");

  if (!matrix9x13.begin(IS3741_ADDR_DEFAULT))
  {
    Serial.println("9x13 not found");
    return;
  }
  Serial.println("9x13 found!");

  // Set brightness to max and bring controller out of shutdown state
  matrix9x13.setLEDscaling(0x10);
  matrix9x13.setGlobalCurrent(0xFF);
  matrix9x13.fill(0);
  matrix9x13.enable(true); // bring out of shutdown
  matrix9x13.setRotation(0);
  matrix9x13.setTextWrap(false);
  matrix9x13.setFont(&Font3x4);

  // Get text dimensions to determine X coord where scrolling resets
  uint16_t w, h;
  int16_t ignore;
  matrix9x13.getTextBounds(matrix9x13Message, 0, 0, &ignore, &ignore, &w, &h);
  text_min = -w; // Off left edge this many pixels
  matrix9x13.getTextBounds(matrix9x13Message2, 0, 0, &ignore, &ignore, &w, &h);
  text_min2 = -w; // Off left edge this many pixels

  xTaskCreate(Matrix9x13Task, "Matrix9x13Task", 4096, NULL, 10, &matrix9x13TaskHandle);
}

void Matrix9x13Task(void *parameters)
{
  bool isEnabled = true;
  while (1)
  {
    if (!display)
    {
      if (isEnabled)
      {
        matrix9x13.enable(false);
        isEnabled = false;
      }
      delay(100);
      continue;
    }
    else if (!isEnabled)
    {
      matrix9x13.enable(true);
      isEnabled = true;
    }

    matrix9x13.fill(0);
    matrix9x13.setCursor(text_x, text_y);
    matrix9x13.setTextColor(matrix9x13.color565(0xFF, 0xA5, 0));
    matrix9x13.print(matrix9x13Message);

    matrix9x13.setCursor(text_x2, text_y2);
    matrix9x13.setTextColor(matrix9x13.color565(0xFF, 0xBF, 0));
    matrix9x13.print(matrix9x13Message2);
    matrix9x13.show();

    if (--text_x < text_min)
      text_x = matrix9x13.width();

    if (--text_x2 < text_min2)
      text_x2 = matrix9x13.width();

    delay(50);
  }
}


    // // bug, need to manually blank background of previous text
    // uint16_t w, h;
    // int16_t x, y;
    // matrix9x13.getTextBounds(matrix9x13Message, rand_x, 5, &x, &y, &w, &h);
    // matrix9x13.fillRect(x, y, w, h, 0);
