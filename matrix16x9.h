#pragma once

#include <Adafruit_IS31FL3731.h>

#include "Font3x4N.h"
#include "matrix_task_handler.h"

extern volatile bool display;

class Matrix16x9TaskHandler : public MatrixTaskHandler
{
private:
  Adafruit_IS31FL3731 matrix = Adafruit_IS31FL3731();
  static const uint8_t DISPLAY_WIDTH = 16;
  static const uint8_t DISPLAY_HEIGHT = 9;
  static constexpr const char *TAG = "Matrix16x9TaskHandler";
  char message[100];
  bool messagePixels[DISPLAY_HEIGHT][DISPLAY_WIDTH];
  // uint8_t sweep[] = {1, 2, 3, 4, 6, 8, 10, 15, 20, 30, 40, 60, 60, 40, 30, 20, 15, 10, 8, 6, 4, 3, 2, 1};
  uint8_t scan[7] = {120, 40, 20, 5, 2, 1, 0};
  int scanX = -1;
  int direction = 1;
  uint8_t scanWidth = 7;

public:
  Matrix16x9TaskHandler() {}
  bool createTask() override;

private:
  void matrixTask(void *parameters) override;
};
