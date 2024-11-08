#pragma once

#include <Adafruit_IS31FL3731.h>

#include "Font3x4N.h"
#include "task_handler.h"

extern volatile bool display;

class Matrix16x9TaskHandler : public TaskHandler
{
private:
    static const uint8_t WIDTH = 16;
    static const uint8_t HEIGHT = 9;
    static constexpr const char *TAG = "Matrix16x9TaskHandler";
    static const uint8_t ScanWidth = 7;

    uint8_t ScanVals[7] = {120, 40, 20, 5, 2, 1, 0};
    // uint8_t sweep[] = {1, 2, 3, 4, 6, 8, 10, 15, 20, 30, 40, 60, 60, 40, 30, 20, 15, 10, 8, 6, 4, 3, 2, 1};

    Adafruit_IS31FL3731 _matrix = Adafruit_IS31FL3731();
    bool _isNewMessage = false;
    bool _messagePixels[HEIGHT][WIDTH];
    int _direction = 1;
    int _scanX = -1;

public:
    Matrix16x9TaskHandler() {}
    bool createTask() override;
    void setMessage(const char *message);

private:
    void task(void *parameters) override;

    void initializeMessagePixels();
    void setCharPixels(int16_t x, int16_t y, char c, uint8_t &glyphWidth);
};

bool Matrix16x9TaskHandler::createTask()
{
    if (_taskHandle != NULL)
    {
        log_w("Task already started");
        return false;
    }

    if (!_matrix.begin())
    {
        log_e("Matrix not found");
        return false;
    }

    _matrix.setFont(&Font3x4N);
    strcpy(_message, "123");
    initializeMessagePixels();

    xTaskCreate(taskWrapper, "Matrix16x9Task", 4096, this, 1, &_taskHandle); // highest priority of tasks for smooth animation: TODO: parameterize
    log_d("Matrix initialized and task started");

    return true;
}

void Matrix16x9TaskHandler::setMessage(const char *message)
{
    TaskHandler::setMessage(message);
    _isNewMessage = true; // signals to task to reinitialize message after scanning
}

void Matrix16x9TaskHandler::task(void *parameters)
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
            if (_isNewMessage)
            {
                initializeMessagePixels();
            }
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
                uint16_t brightness = _messagePixels[y][x] && _direction > 0 && scanIndex > 2
                                          ? ScanVals[2]
                                          : ScanVals[scanIndex];

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

void Matrix16x9TaskHandler::initializeMessagePixels()
{
    if (!_isNewMessage)
    {
        log_w("Message already initialized");
        return;
    }

    memset(_messagePixels, 0, sizeof(_messagePixels));

    // iterate over the message and set the pixels
    int cursorX = 3;
    int cursorY = 6;
    String message = String(_message);
    for (int i = 0; i < message.length(); i++)
    {
        uint8_t charWidth;
        setCharPixels(cursorX, cursorY, message[i], charWidth);
        cursorX += charWidth + 1;
    }

    _isNewMessage = false;
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
                int xc = x + xo + xx;
                int yc = y + yo + yy;
                if (xc >= 0 && xc < WIDTH && yc >= 0 && yc < HEIGHT)
                {
                    _messagePixels[yc][xc] = true;
                }
            }
            bits <<= 1;
        }
    }

    glyphWidth = w;
}
