#include "matrix16x9.h"

bool Matrix16x9TaskHandler::createTask()
{
    if (taskHandle != NULL)
    {
        log_w("Task already started");
        return false;
    }

    strcpy(message, "ABC");
    if (!matrix.begin())
    {
        log_e("Matrix not found");
        return false;
    }

    // matrix16x9.setFont(&Font3x4N);
    // initializeMessage();

    xTaskCreate(matrixTaskWrapper, "Matrix16x9Task", 4096, this, 1, &taskHandle);
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
            matrix.clear();
            delay(100);
            continue;
        }

        if (direction > 0 && scanX >= DISPLAY_WIDTH + scanWidth)
        {
            scanX = DISPLAY_WIDTH;
            direction = -1;
        }
        else if (direction < 0 && scanX <= -scanWidth)
        {
            scanX = -1;
            direction = 1;
        }

        for (int scanIndex = 0; scanIndex < scanWidth; scanIndex++)
        {
            int x = scanX - (direction * scanIndex);
            if (x < 0 || x >= DISPLAY_WIDTH)
            {
                continue;
            }

            for (int y = 0; y < DISPLAY_HEIGHT; y++)
            {
                // matrix16x9.drawPixel(x, y, scan[scanIndex]);
                uint16_t brightness = scan[scanIndex];
                if (messagePixels[y][x])
                {
                    brightness = brightness > 0 ? 150 : 0;
                }
                matrix.drawPixel(x, y, brightness);
            }
        }
        scanX += direction;

        // for (uint8_t incr = 0; incr < 24; incr++)
        //   for (uint8_t x = 0; x < 16; x++)
        //     for (uint8_t y = 0; y < 9; y++)
        //       matrix16x9.drawPixel(x, y, sweep[(x + y + incr) % 24]);
        delay(30);
    }
}

// void Matrix16x9TaskHandler::setCharPixels(int16_t x, int16_t y, char c, uint8_t &glyphWidth)
// {
//   c -= (uint8_t)pgm_read_byte(&Font3x4N.first); // Adjust the character index

//   GFXglyph *glyph = &(((GFXglyph *)pgm_read_ptr(&Font3x4N.glyph))[c]);
//   uint8_t *bitmap = (uint8_t *)pgm_read_ptr(&Font3x4N.bitmap);

//   uint16_t bo = pgm_read_word(&glyph->bitmapOffset);
//   uint8_t w = pgm_read_byte(&glyph->width);
//   uint8_t h = pgm_read_byte(&glyph->height);
//   int8_t xo = pgm_read_byte(&glyph->xOffset);
//   int8_t yo = pgm_read_byte(&glyph->yOffset);
//   uint8_t xx, yy, bits = 0, bit = 0;

//   for (yy = 0; yy < h; yy++)
//   {
//     for (xx = 0; xx < w; xx++)
//     {
//       if (!(bit++ & 7))
//       {
//         bits = pgm_read_byte(&bitmap[bo++]);
//       }
//       if (bits & 0x80)
//       {
//         int x = x + xo + xx;
//         int y = y + yo + yy;
//         if (x >= 0 && x < DISPLAY_WIDTH && y >= 0 && y < DISPLAY_HEIGHT)
//         {
//           messagePixels[y][x] = true;
//         }
//       }
//       bits <<= 1;
//     }
//   }

//   glyphWidth = w;
// }

// void Matrix16x9TaskHandler::initializeMessage()
// {
//   memset(messagePixels, 0, sizeof(messagePixels));
//   // for (int y = 0; y < DISPLAY_HEIGHT; y++)
//   // {
//   //   for (int x = 0; x < DISPLAY_WIDTH; x++)
//   //   {
//   //     messagePixels[y][x] = false;
//   //   }
//   // }

//   // iterate over the message and set the pixels
//   int cursorX = 1;
//   int cursorY = 1;
//   String message = String(matrix16x9Message);
//   for (int i = 0; i < message.length(); i++)
//   {
//     uint8_t charWidth;
//     setCharPixels(cursorX, cursorY, message[i], charWidth);
//     cursorX += charWidth;
//   }
// }
