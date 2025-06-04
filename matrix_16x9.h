#pragma once

#include <Adafruit_IS31FL3731.h>

#include "Font3x4N.h"
#include "display_task_handler.h"

static constexpr const uint8_t Gamma[256] = {
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 1, 1, 1, 2, 2, 2,
    2, 2, 2, 3, 3, 3, 3, 3, 4, 4, 4, 4, 5, 5, 5, 5, 6, 6, 6, 7, 7, 7, 8, 8, 8, 9, 9, 9, 10, 10, 11,
    11, 11, 12, 12, 13, 13, 13, 14, 14, 15, 15, 16, 16, 17, 17, 18, 18, 19, 19, 20, 21, 21, 22, 22,
    23, 23, 24, 25, 25, 26, 27, 27, 28, 29, 29, 30, 31, 31, 32, 33, 34, 34, 35, 36, 37, 37, 38, 39,
    40, 40, 41, 42, 43, 44, 45, 46, 46, 47, 48, 49, 50, 51, 52, 53, 54, 55, 56, 57, 58, 59, 60, 61,
    62, 63, 64, 65, 66, 67, 68, 69, 70, 71, 72, 73, 74, 76, 77, 78, 79, 80, 81, 83, 84, 85, 86, 88,
    89, 90, 91, 93, 94, 95, 96, 98, 99, 100, 102, 103, 104, 106, 107, 109, 110, 111, 113, 114, 116,
    117, 119, 120, 121, 123, 124, 126, 128, 129, 131, 132, 134, 135, 137, 138, 140, 142, 143, 145,
    146, 148, 150, 151, 153, 155, 157, 158, 160, 162, 163, 165, 167, 169, 170, 172, 174, 176, 178,
    179, 181, 183, 185, 187, 189, 191, 193, 194, 196, 198, 200, 202, 204, 206, 208, 210, 212, 214,
    216, 218, 220, 222, 224, 227, 229, 231, 233, 235, 237, 239, 241, 244, 246, 248, 250, 252, 255};

class Matrix16x9TaskHandler : public DisplayTaskHandler
{
private:
    static const uint8_t I2C_ADDR = ISSI_ADDR_DEFAULT;
    static const uint8_t TASK_PRIORITY = 10; // highest priority of matrix tasks for smooth animation
    static const uint8_t WIDTH = 16;
    static const uint8_t HEIGHT = 9;
    static const uint8_t NUM_STARS = 100;
    static constexpr const char *TAG = "Matrix16x9TaskHandler";
    static const uint8_t ScanWidth = 7;
    uint8_t ScanVals[7] = {60, 20, 8, 4, 2, 1, 0};
    // uint8_t sweep[] = {1, 2, 3, 4, 6, 8, 10, 15, 20, 30, 40, 60, 60, 40, 30, 20, 15, 10, 8, 6, 4, 3, 2, 1};

    Adafruit_IS31FL3731 _matrix = Adafruit_IS31FL3731();
    bool _isNewMessage = false;
    // bool _messagePixels[HEIGHT][WIDTH];
    // int _direction = 1;
    // int _scanX = -1;

    class Star
    {
    private:
        const float Speed = 0.3f;            // speed of star movement
        const float OriginX = WIDTH / 2.0f;  // origin point for projection
        const float OriginY = HEIGHT / 2.0f; // origin point for projection

        static const uint8_t MAX_DISTANCE = 50;

        float x, y, z;

    public:
        Star()
        {
            reset(false);
        }

        void project(int16_t &x, int16_t &y, int16_t &brightness)
        {
            float k = 8.0 / z;
            x = static_cast<int16_t>(this->x * k + OriginX);
            y = static_cast<int16_t>(this->y * k + OriginY);
            float norm = 1.0f - (this->z / MAX_DISTANCE);
            float expNorm = powf(norm, 3.0f);
            brightness = Gamma[static_cast<int16_t>(255 * expNorm)];
        }

        void reset(bool maxDistance = true)
        {
            x = (rand() % WIDTH) - OriginX;
            y = (rand() % HEIGHT) - OriginY;
            z = maxDistance ? MAX_DISTANCE : rand() % MAX_DISTANCE;
        }

        void update()
        {
            z -= Speed;
            if (z <= 0)
            {
                reset();
            }
        }
    };

    Star _stars[NUM_STARS];
    uint8_t _starfield[WIDTH][HEIGHT] = {0};

public:
    Matrix16x9TaskHandler() {}
    bool createTask() override;
    void setMessage(const char *message);

private:
    void task(void *parameters) override;

    // void initializeMessagePixels();
    // void setCharPixels(int16_t x, int16_t y, char c, uint8_t &glyphWidth);
};

bool Matrix16x9TaskHandler::createTask()
{
    if (_taskHandle != NULL)
    {
        log_w("Task already started");
        return false;
    }

    if (!_matrix.begin(I2C_ADDR))
    {
        log_e("Matrix not found");
        return false;
    }

    //_matrix.setFont(&Font3x4N);
    // strcpy(_message, "123");
    // initializeMessagePixels();

    xTaskCreate(taskWrapper, "Matrix16x9Task", 4096, this, TASK_PRIORITY, &_taskHandle);
    log_d("Matrix initialized and task started");

    return true;
}

void Matrix16x9TaskHandler::setMessage(const char *message)
{
    DisplayTaskHandler::setMessage(message);
    _isNewMessage = true; // signals to task to reinitialize message after scanning
}

void Matrix16x9TaskHandler::task(void *parameters)
{
    log_d("Starting Matrix16x9Task");

    while (1)
    {
        if (!_display)
        {
            _matrix.clear();
            delay(100);
            continue;
        }

        // build a new frame of starfield
        uint8_t newStarfield[WIDTH][HEIGHT] = {0};
        for (int i = 0; i < NUM_STARS; i++)
        {
            int16_t x, y, b;
            _stars[i].update();
            _stars[i].project(x, y, b);
            if (x < 0 || x >= WIDTH || y < 0 || y >= HEIGHT)
                continue; // skip out of bounds stars
            newStarfield[x][y] = b > newStarfield[x][y] ? b : newStarfield[x][y];
        }

        // iterate over existing starfield and update only changed pixels
        for (int x = 0; x < WIDTH; x++)
        {
            for (int y = 0; y < HEIGHT; y++)
            {
                if (_starfield[x][y] != newStarfield[x][y])
                {
                    _starfield[x][y] = newStarfield[x][y];
                    _matrix.drawPixel(x, y, _starfield[x][y]);
                }
            }
        }

        //_matrix.drawPixel(x, y, b);

        // if (_direction > 0 && _scanX >= WIDTH + ScanWidth)
        // {
        //     _scanX = WIDTH;
        //     _direction = -1;
        // }
        // else if (_direction < 0 && _scanX <= -ScanWidth)
        // {
        //     _scanX = -1;
        //     _direction = 1;
        //     if (_isNewMessage)
        //     {
        //         initializeMessagePixels();
        //     }
        // }

        // for (int scanIndex = 0; scanIndex < ScanWidth; scanIndex++)
        // {
        //     int x = _scanX - (_direction * scanIndex);
        //     if (x < 0 || x >= WIDTH)
        //     {
        //         continue;
        //     }

        //     for (int y = 0; y < HEIGHT; y++)
        //     {
        //         // matrix16x9.drawPixel(x, y, scan[scanIndex]);
        //         uint16_t brightness = _messagePixels[y][x] && _direction > 0 && scanIndex > 2
        //                                   ? ScanVals[2]
        //                                   : ScanVals[scanIndex];

        //         _matrix.drawPixel(x, y, brightness);
        //     }
        // }
        // _scanX += _direction;

        // for (uint8_t incr = 0; incr < 24; incr++)
        //   for (uint8_t x = 0; x < 16; x++)
        //     for (uint8_t y = 0; y < 9; y++)
        //       matrix16x9.drawPixel(x, y, sweep[(x + y + incr) % 24]);
        delay(30);
    }
}

// void Matrix16x9TaskHandler::initializeMessagePixels()
// {
//     if (!_isNewMessage)
//     {
//         log_w("Message already initialized");
//         return;
//     }

//     memset(_messagePixels, 0, sizeof(_messagePixels));

//     // iterate over the message and set the pixels
//     int cursorX = 3;
//     int cursorY = 6;
//     String message = String(_message);
//     for (int i = 0; i < message.length(); i++)
//     {
//         uint8_t charWidth;
//         setCharPixels(cursorX, cursorY, message[i], charWidth);
//         cursorX += charWidth + 1;
//     }

//     _isNewMessage = false;
// }

// void Matrix16x9TaskHandler::setCharPixels(int16_t x, int16_t y, char c, uint8_t &glyphWidth)
// {
//     c -= (uint8_t)pgm_read_byte(&Font3x4N.first); // Adjust the character index

//     GFXglyph *glyph = &(((GFXglyph *)pgm_read_ptr(&Font3x4N.glyph))[c]);
//     uint8_t *bitmap = (uint8_t *)pgm_read_ptr(&Font3x4N.bitmap);

//     uint16_t bo = pgm_read_word(&glyph->bitmapOffset);
//     uint8_t w = pgm_read_byte(&glyph->width);
//     uint8_t h = pgm_read_byte(&glyph->height);
//     int8_t xo = pgm_read_byte(&glyph->xOffset);
//     int8_t yo = pgm_read_byte(&glyph->yOffset);
//     uint8_t xx, yy, bits = 0, bit = 0;

//     for (yy = 0; yy < h; yy++)
//     {
//         for (xx = 0; xx < w; xx++)
//         {
//             if (!(bit++ & 7))
//             {
//                 bits = pgm_read_byte(&bitmap[bo++]);
//             }
//             if (bits & 0x80)
//             {
//                 int xc = x + xo + xx;
//                 int yc = y + yo + yy;
//                 if (xc >= 0 && xc < WIDTH && yc >= 0 && yc < HEIGHT)
//                 {
//                     _messagePixels[yc][xc] = true;
//                 }
//             }
//             bits <<= 1;
//         }
//     }

//     glyphWidth = w;
// }
