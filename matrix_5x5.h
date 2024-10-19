#include <Adafruit_NeoMatrix.h>
#include <Fonts/TomThumb.h>

#include "matrix_task_handler.h"

extern volatile bool display;

Adafruit_NeoMatrix _matrix(5, 5, A3,
                            NEO_MATRIX_BOTTOM + NEO_MATRIX_RIGHT + NEO_MATRIX_COLUMNS + NEO_MATRIX_PROGRESSIVE,
                            NEO_GRB + NEO_KHZ800);
const uint16_t colors[3] = {
    _matrix.Color(255, 55, 0), _matrix.Color(255, 110, 0), _matrix.Color(255, 165, 0)};

class Matrix5x5TaskHandler : public MatrixTaskHandler
{
private:
    static const uint8_t PIN = A3;
    static const uint8_t WIDTH = 5;
    static const uint8_t HEIGHT = 5;

    uint16_t _message_width;  // Computed in setup() below
    uint8_t _x = _matrix.width();  // Start with message off right edge
    uint8_t _y = _matrix.height(); // With custom fonts, y is the baseline, not top
    int _pass = 0;            // Counts through the colors[] array

public:
    Matrix5x5TaskHandler() {}
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

    _matrix.begin();
    _matrix.setBrightness(15);
    _matrix.setFont(&TomThumb);
    _matrix.setTextWrap(false);
    _matrix.setTextColor(colors[0]);

    int16_t d1;
    uint16_t d2;
    _matrix.getTextBounds(_message, 0, 0, &d1, &d1, &_message_width, &d2);

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
            _matrix.fillScreen(0);
            _matrix.show();
            delay(100);
            continue;
        }

        _matrix.fillScreen(0);
        _matrix.setCursor(_x, _y);
        _matrix.print(_message);
        _matrix.show();

        if (--_x < -_message_width)
        {
            _x = _matrix.width();
            if (++_pass >= 3)
                _pass = 0;
            _matrix.setTextColor(colors[_pass]);
        }

        delay(100);
    }
}
