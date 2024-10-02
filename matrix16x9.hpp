#include <Adafruit_IS31FL3731.h>

extern volatile bool display;
TaskHandle_t matrix16x9TaskHandle = NULL;

Adafruit_IS31FL3731 matrix16x9 = Adafruit_IS31FL3731();
uint8_t sweep[] = {1, 2, 3, 4, 6, 8, 10, 15, 20, 30, 40, 60, 60, 40, 30, 20, 15, 10, 8, 6, 4, 3, 2, 1};
//uint8_t scan[] = {0, 0, 0, 0, 1, 2, 3, 5, 10, 20, 40, 80};
uint8_t scan[] = {120, 40, 20, 5, 2, 1, 0};
int scanX = -1;
int direction = 1;
uint8_t scanWidth = 7;

const uint8_t DISPLAY_WIDTH = 16;
const uint8_t DISPLAY_HEIGHT = 9;

void Matrix16x9Task(void *parameters);

void Matrix16x9Setup()
{
  if (!matrix16x9.begin())
  {
    log_e("16x9 not found");
    return;
  }

  xTaskCreate(Matrix16x9Task, "Matrix16x9Task", 4096, NULL, 1, &matrix16x9TaskHandle);
  log_d("16x9 set up complete");
}

void Matrix16x9Task(void *parameters)
{
  log_d("Starting Matrix16x9Task");

  while (1)
  {
    if (!display)
    {
      matrix16x9.clear();
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
        matrix16x9.drawPixel(x, y, scan[scanIndex]);
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
