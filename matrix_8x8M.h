
#pragma once

#include <Adafruit_LEDBackpack.h>

#include "display_task_handler.h"


class Matrix8x8MTaskHandler : public DisplayTaskHandler
{
private:
  static const uint8_t I2C_ADDR = 0x71;
  static const uint8_t TASK_PRIORITY = 5;
  static const uint8_t REFRESH_TIME = 15;
  static const uint8_t BASE_DELAY = 20;
  static const uint8_t WIDTH = 8;
  static const uint8_t HEIGHT = 8;

  struct BlinkenPixel
  {
    bool isOn;
    int delay;
  };

  Adafruit_8x8matrix matrix = Adafruit_8x8matrix();
  BlinkenPixel pixelData[HEIGHT][WIDTH];

public:
  Matrix8x8MTaskHandler() {}
  bool createTask() override;

private:
  void task(void *parameters) override;
};


bool Matrix8x8MTaskHandler::createTask()
{
  if (_taskHandle != NULL)
  {
    log_w("Task already started");
    return false;
  }

  if (!matrix.begin(I2C_ADDR))
  {
    log_e("Matrix not found");
    return false;
  }

  matrix.setBrightness(2);

  for (int y = 0; y < HEIGHT; y++)
  {
    for (int x = 0; x < WIDTH; x++)
    {
      pixelData[y][x].isOn = true;
      pixelData[y][x].delay = random(BASE_DELAY * 5, BASE_DELAY * 10);

      matrix.drawPixel(x, y, true);
    }
  }

  xTaskCreate(taskWrapper, "Matrix8x8MTask", 4096, this, TASK_PRIORITY, &_taskHandle);
  log_d("Matrix initialized and task started");

  return true;
}

void Matrix8x8MTaskHandler::task(void *parameters)
{
  log_d("Starting Matrix8x8MTask");

  while (1)
  {
    if (!_display)
    {
      matrix.fillScreen(LED_OFF);
      matrix.writeDisplay();
      delay(100);
      continue;
    }

    for (int y = 0; y < HEIGHT; y++)
    {
      for (int x = 0; x < WIDTH; x++)
      {
        if (--pixelData[y][x].delay > 0)
          continue;

        pixelData[y][x].isOn = !pixelData[y][x].isOn;
        pixelData[y][x].delay = random(BASE_DELAY, BASE_DELAY * 2);

        matrix.drawPixel(x, y, pixelData[y][x].isOn);
      }
    }

    matrix.writeDisplay();
    delay(REFRESH_TIME);
  }
}
