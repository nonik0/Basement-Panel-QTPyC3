#pragma once
#include <Arduino.h>

class MatrixTaskHandler
{
protected:
    TaskHandle_t _taskHandle = NULL;
    char _message[100];

public:
    virtual bool createTask() = 0;

    virtual void setMessage(const char *message) { strcpy(_message, message); }

    bool suspendTask()
    {
        if (_taskHandle != NULL)
        {
            vTaskSuspend(_taskHandle);
            return true;
        }
        return false;
    }

protected:
    virtual void matrixTask(void *parameters) = 0;

    static void matrixTaskWrapper(void *parameters)
    {
        MatrixTaskHandler *handler = static_cast<MatrixTaskHandler *>(parameters);
        handler->matrixTask(parameters);
    }
};