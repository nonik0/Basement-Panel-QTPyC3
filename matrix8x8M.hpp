#include <Adafruit_LEDBackpack.h>

#define REFRESHMTIME 15
#define MATRIXMWIDTH 8
#define MATRIXMHEIGHT 8
#define BASEMDELAY 20

extern volatile bool display;
TaskHandle_t matrix8x8MTaskHandle = NULL;
Adafruit_8x8matrix matrix8x8M = Adafruit_8x8matrix();

void Matrix8x8MTask(void *parameters);

struct BlinkenPixel
{
  bool isOn;
  int delay;
};
BlinkenPixel pixelData[MATRIXMHEIGHT][MATRIXMWIDTH];

void Matrix8x8MSetup()
{
  if (!matrix8x8M.begin(0x71))
  {
    log_e("8x8M not found");
    return;
  }

  matrix8x8M.setBrightness(5);

  for (int y = 0; y < MATRIXMHEIGHT; y++)
  {
    for (int x = 0; x < MATRIXMWIDTH; x++)
    {
      pixelData[y][x].isOn = true;
      pixelData[y][x].delay = random(BASEMDELAY * 5, BASEMDELAY * 10);

      matrix8x8M.drawPixel(x, y, true);
    }
  }

  xTaskCreate(Matrix8x8MTask, "Matrix8x8MTask", 4096, NULL, 2, &matrix8x8MTaskHandle);
  log_d("8x8M set up complete");
}

void Matrix8x8MTask(void *parameters)
{
  log_d("Starting Matrix8x8MTask");

  while (1)
  {
    if (!display)
    {
      matrix8x8M.fillScreen(LED_OFF);
      matrix8x8M.writeDisplay();
      delay(100);
      continue;
    }

    for (int y = 0; y < MATRIXMHEIGHT; y++)
    {
      for (int x = 0; x < MATRIXMWIDTH; x++)
      {
        if (--pixelData[y][x].delay > 0)
          continue;

        pixelData[y][x].isOn = !pixelData[y][x].isOn;
        pixelData[y][x].delay = random(BASEMDELAY, BASEMDELAY * 2);

        matrix8x8M.drawPixel(x, y, pixelData[y][x].isOn);
      }
    }

    matrix8x8M.writeDisplay();
    delay(REFRESHMTIME);
  }
}