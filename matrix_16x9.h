#pragma once

#include <Adafruit_IS31FL3731.h>

#include "Font3x4N.h"
#include "matrix_task_handler.h"

extern volatile bool display;

class Matrix16x9TaskHandler : public MatrixTaskHandler
{
private:
  static const uint8_t WIDTH = 16;
  static const uint8_t HEIGHT = 9;
  static constexpr const char *TAG = "Matrix16x9TaskHandler";
  static const uint8_t ScanWidth = 7;
  
  uint8_t ScanVals[7] = {120, 40, 20, 5, 2, 1, 0};
  // uint8_t sweep[] = {1, 2, 3, 4, 6, 8, 10, 15, 20, 30, 40, 60, 60, 40, 30, 20, 15, 10, 8, 6, 4, 3, 2, 1};
  
  Adafruit_IS31FL3731 _matrix = Adafruit_IS31FL3731();
  bool _messagePixels[HEIGHT][WIDTH];
  int _direction = 1;
  int _scanX = -1;

public:
  Matrix16x9TaskHandler() {}
  bool createTask() override;

private:
  void matrixTask(void *parameters) override;

  void initializeMessage();
  void setCharPixels(int16_t x, int16_t y, char c, uint8_t &glyphWidth);
};


bool Matrix16x9TaskHandler::createTask()
{
    if (_taskHandle != NULL)
    {
        log_w("Task already started");
        return false;
    }

    strcpy(_message, "ABC");
    if (!_matrix.begin())
    {
        log_e("Matrix not found");
        return false;
    }

    // matrix16x9.setFont(&Font3x4N);
    // initializeMessage();

    xTaskCreate(matrixTaskWrapper, "Matrix16x9Task", 4096, this, 1, &_taskHandle);
    log_d("Matrix initialized and task started");

    return true;
}

void Matrix16x9TaskHandler::matrixTask(void *parameters)
{
    log_d("Starting Matrix16x9Task");

    while (1)
    {
        if (!display)
        {
            _matrix.clear();
            delay(100);
            continue;
        }

        if (_direction > 0 && _scanX >= WIDTH + ScanWidth)
        {
            _scanX = WIDTH;
            _direction = -1;
        }
        else if (_direction < 0 && _scanX <= -ScanWidth)
        {
            _scanX = -1;
            _direction = 1;
        }

        for (int scanIndex = 0; scanIndex < ScanWidth; scanIndex++)
        {
            int x = _scanX - (_direction * scanIndex);
            if (x < 0 || x >= WIDTH)
            {
                continue;
            }

            for (int y = 0; y < HEIGHT; y++)
            {
                // matrix16x9.drawPixel(x, y, scan[scanIndex]);
                uint16_t brightness = ScanVals[scanIndex];
                if (_messagePixels[y][x])
                {
                    brightness = brightness > 0 ? 150 : 0;
                }
                _matrix.drawPixel(x, y, brightness);
            }
        }
        _scanX += _direction;

        // for (uint8_t incr = 0; incr < 24; incr++)
        //   for (uint8_t x = 0; x < 16; x++)
        //     for (uint8_t y = 0; y < 9; y++)
        //       matrix16x9.drawPixel(x, y, sweep[(x + y + incr) % 24]);
        delay(30);
    }
}

void Matrix16x9TaskHandler::initializeMessage()
{
  memset(_messagePixels, 0, sizeof(_messagePixels));

  // iterate over the message and set the pixels
  int cursorX = 1;
  int cursorY = 1;
  String message = String(_message);
  for (int i = 0; i < message.length(); i++)
  {
    uint8_t charWidth;
    setCharPixels(cursorX, cursorY, message[i], charWidth);
    cursorX += charWidth;
  }
}

void Matrix16x9TaskHandler::setCharPixels(int16_t x, int16_t y, char c, uint8_t &glyphWidth)
{
  c -= (uint8_t)pgm_read_byte(&Font3x4N.first); // Adjust the character index

  GFXglyph *glyph = &(((GFXglyph *)pgm_read_ptr(&Font3x4N.glyph))[c]);
  uint8_t *bitmap = (uint8_t *)pgm_read_ptr(&Font3x4N.bitmap);

  uint16_t bo = pgm_read_word(&glyph->bitmapOffset);
  uint8_t w = pgm_read_byte(&glyph->width);
  uint8_t h = pgm_read_byte(&glyph->height);
  int8_t xo = pgm_read_byte(&glyph->xOffset);
  int8_t yo = pgm_read_byte(&glyph->yOffset);
  uint8_t xx, yy, bits = 0, bit = 0;

  for (yy = 0; yy < h; yy++)
  {
    for (xx = 0; xx < w; xx++)
    {
      if (!(bit++ & 7))
      {
        bits = pgm_read_byte(&bitmap[bo++]);
      }
      if (bits & 0x80)
      {
        int x = x + xo + xx;
        int y = y + yo + yy;
        if (x >= 0 && x < WIDTH && y >= 0 && y < HEIGHT)
        {
          _messagePixels[y][x] = true;
        }
      }
      bits <<= 1;
    }
  }

  glyphWidth = w;
}

