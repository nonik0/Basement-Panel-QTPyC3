#include <Adafruit_IS31FL3731.h>

extern volatile bool display;
TaskHandle_t matrix16x9TaskHandle = NULL;

Adafruit_IS31FL3731 matrix16x9 = Adafruit_IS31FL3731();
uint8_t sweep[] = {1, 2, 3, 4, 6, 8, 10, 15, 20, 30, 40, 60, 60, 40, 30, 20, 15, 10, 8, 6, 4, 3, 2, 1};

void Matrix16x9Task(void *parameters);

void Matrix16x9Setup()
{
  if (!matrix16x9.begin())
  {
    Serial.println("16x9 not found");
    return;
  }
  Serial.println("16x9 found!");

  xTaskCreate(Matrix16x9Task, "Matrix16x9Task", 4096, NULL, 1, &matrix16x9TaskHandle);
}

void Matrix16x9Task(void *parameters)
{
  while (1)
  {
    if (!display)
    {
      matrix16x9.clear();
      delay(100);
      continue;
    }

    for (uint8_t incr = 0; incr < 24; incr++)
      for (uint8_t x = 0; x < 16; x++)
        for (uint8_t y = 0; y < 9; y++)
          matrix16x9.drawPixel(x, y, sweep[(x + y + incr) % 24]);
    delay(30);
  }
}

