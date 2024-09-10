#include <Adafruit_IS31FL3731.h>

TaskHandle_t matrix9x16TaskHandle = NULL;

Adafruit_IS31FL3731 matrix9x16 = Adafruit_IS31FL3731();
uint8_t sweep[] = {1, 2, 3, 4, 6, 8, 10, 15, 20, 30, 40, 60, 60, 40, 30, 20, 15, 10, 8, 6, 4, 3, 2, 1};

void Matrix9x16Task(void *parameters);

void Matrix9x16Setup()
{
  if (!matrix9x16.begin())
  {
    Serial.println("9x16 not found");
    return;
  }
  Serial.println("9x16 found!");

  xTaskCreate(Matrix9x16Task, "Matrix9x16Task", 4096, NULL, 10, &matrix9x16TaskHandle);
}

void Matrix9x16Task(void *parameters)
{
  while (1)
  {
    if (!display)
    {
      matrix9x16.clear();
      delay(100);
      continue;
    }

    for (uint8_t incr = 0; incr < 24; incr++)
      for (uint8_t x = 0; x < 16; x++)
        for (uint8_t y = 0; y < 9; y++)
          matrix9x16.drawPixel(x, y, sweep[(x + y + incr) % 24]);
    delay(20);
  }
}

