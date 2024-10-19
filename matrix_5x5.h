#include <Adafruit_NeoMatrix.h>
#include <Fonts/TomThumb.h>

#include "matrix_task_handler.h"

extern volatile bool display;

Adafruit_NeoMatrix matrix(5, 5, A3,
                            NEO_MATRIX_BOTTOM + NEO_MATRIX_RIGHT + NEO_MATRIX_COLUMNS + NEO_MATRIX_PROGRESSIVE,
                            NEO_GRB + NEO_KHZ800);
const uint16_t colors[3] = {
    matrix.Color(255, 55, 0), matrix.Color(255, 110, 0), matrix.Color(255, 165, 0)};

class Matrix5x5TaskHandler : public MatrixTaskHandler
{
private:
    static const int PIN = A3;
    static const int WIDTH = 5;
    static const int HEIGHT = 5;


    uint16_t message_width;  // Computed in setup() below
    int x = matrix.width();  // Start with message off right edge
    int y = matrix.height(); // With custom fonts, y is the baseline, not top
    int pass = 0;            // Counts through the colors[] array

public:
    Matrix5x5TaskHandler()
    {
        x = matrix.width();  // Start with message off right edge
        y = matrix.height(); // With custom fonts, y is the baseline, not top
        pass = 0;            // Counts through the colors[] array
    }
    bool createTask() override;

private:
    void matrixTask(void *parameters) override;
};

bool Matrix5x5TaskHandler::createTask()
{
  if (_taskHandle != NULL)
  {
    log_w("Task already started");
    return false;
  }

    strcpy(_message, "BEAU IN TOW!");

    matrix.begin();
    matrix.setBrightness(15);
    matrix.setFont(&TomThumb);
    matrix.setTextWrap(false);
    matrix.setTextColor(colors[0]);

    int16_t d1;
    uint16_t d2;
    matrix.getTextBounds(_message, 0, 0, &d1, &d1, &message_width, &d2);

    xTaskCreate(matrixTaskWrapper, "Matrix5x5Task", 4096, this, 2, &_taskHandle);
    log_d("5x5 set up complete");

    return true;
}

void Matrix5x5TaskHandler::matrixTask(void *parameters) 
{
    log_d("Starting Matrix5x5Task");

    while (1)
    {
        if (!display)
        {
            matrix.fillScreen(0);
            matrix.show();
            delay(100);
            continue;
        }

        matrix.fillScreen(0);
        matrix.setCursor(x, y);
        matrix.print(_message);
        matrix.show();

        if (--x < -message_width)
        {
            x = matrix.width();
            if (++pass >= 3)
                pass = 0;
            matrix.setTextColor(colors[pass]);
        }

        delay(100);
    }
}
