#pragma once

#include <Adafruit_LEDBackpack.h>

#include "display_task_handler.h"
#include "maze_runner_lib.h"

class Matrix8x8TaskHandler : public DisplayTaskHandler
{
private:
  static const int MAZE_DELAY_MS = 50;
  static const uint8_t I2C_ADDR = 0x70;
  static const uint8_t TASK_PRIORITY = 5;
  static const uint8_t WIDTH = 8;
  static const uint8_t HEIGHT = 8;

  Adafruit_BicolorMatrix _matrix = Adafruit_BicolorMatrix();
  MazeRunner *_mazeRunner;

public:
  Matrix8x8TaskHandler() {}
  bool createTask() override;

private:
  void task(void *parameters) override;
};

bool Matrix8x8TaskHandler::createTask()
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

  _matrix.setBrightness(5);

  _mazeRunner = new MazeRunner(
      WIDTH,
      HEIGHT,
      LED_OFF,    // off
      LED_YELLOW, // wall
      LED_GREEN,  // runner
      LED_RED,    // exit
      [this](int x, int y, uint32_t c)
      { _matrix.writePixel(x, y, c); });

  _mazeRunner->init();

  xTaskCreate(taskWrapper, "Matrix8x8Task", 4096, this, TASK_PRIORITY, &_taskHandle);

  log_d("Matrix setup complete");

  return true;
}

void Matrix8x8TaskHandler::task(void *parameters)
{
  log_d("Starting Matrix8x8Task");

  while (1)
  {
    if (!_display)
    {
      _matrix.fillScreen(LED_OFF);
      _matrix.writeDisplay();
      delay(MAZE_DELAY_MS);
      continue;
    }

    if (_mazeRunner->update())
    {
      _matrix.writeDisplay();
    }
    delay(MAZE_DELAY_MS);
  }
}