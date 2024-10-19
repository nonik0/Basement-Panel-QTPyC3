
#pragma once

#include <Adafruit_LEDBackpack.h>

#include "matrix_task_handler.h"

extern volatile bool display;

class Matrix8x8MTaskHandler : public MatrixTaskHandler
{
private:
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
  void matrixTask(void *parameters) override;
};


bool Matrix8x8MTaskHandler::createTask()
{
  if (_taskHandle != NULL)
  {
    log_w("Task already started");
    return false;
  }

  if (!matrix.begin(0x71))
  {
    log_e("Matrix not found");
    return false;
  }

  matrix.setBrightness(5);

  for (int y = 0; y < HEIGHT; y++)
  {
    for (int x = 0; x < WIDTH; x++)
    {
      pixelData[y][x].isOn = true;
      pixelData[y][x].delay = random(BASE_DELAY * 5, BASE_DELAY * 10);

      matrix.drawPixel(x, y, true);
    }
  }

  xTaskCreate(matrixTaskWrapper, "Matrix8x8MTask", 4096, this, 1, &_taskHandle);
  log_d("Matrix initialized and task started");

  return true;
}

void Matrix8x8MTaskHandler::matrixTask(void *parameters)
{
  log_d("Starting Matrix8x8MTask");

  while (1)
  {
    if (!display)
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
