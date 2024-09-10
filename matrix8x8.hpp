#include <Adafruit_LEDBackpack.h>

#define LED_COUNT 64
#define REFRESHTIME 15
#define BASEDELAY 20

extern volatile bool display;
TaskHandle_t matrix8x8TaskHandle = NULL;

struct Pixel
{
  uint32_t color;
  int delay;
};
Pixel pixelData[LED_COUNT];

Adafruit_BicolorMatrix matrix8x8 = Adafruit_BicolorMatrix();

void Matrix8x8Task(void *parameters);

void Matrix8x8Setup()
{
  if (!matrix8x8.begin(0x70))
  {
    Serial.println("8x8 not found");
    return;
  }
  Serial.println("8x8 found!");

  matrix8x8.setBrightness(5);

  randomSeed(analogRead(0));

  for (int i = 0; i < LED_COUNT; i++)
  {
    pixelData[i].color = LED_YELLOW;
    pixelData[i].delay = random(BASEDELAY * 5, BASEDELAY * 10);

    matrix8x8.drawPixel(i / 8, i % 8, pixelData[i].color);
  }

  xTaskCreate(Matrix8x8Task, "Matrix8x8Task", 4096, NULL, 10, &matrix8x8TaskHandle);
}

void Matrix8x8Task(void *parameters)
{
  while (1)
  {
    if (!display)
    {
      matrix8x8.fillScreen(LED_OFF);
      matrix8x8.writeDisplay();
      // matrix8x8.clear();
      delay(100);
      continue;
    }

    for (int i = 0; i < LED_COUNT; i++)
    {
      if (--pixelData[i].delay > 0)
        continue;

      pixelData[i].color = pixelData[i].color == LED_OFF ? LED_YELLOW : LED_OFF;
      pixelData[i].delay = random(BASEDELAY, BASEDELAY * 2);

      matrix8x8.drawPixel(i / 8, i % 8, pixelData[i].color);
    }

    matrix8x8.writeDisplay();
    delay(REFRESHTIME);
  }
}