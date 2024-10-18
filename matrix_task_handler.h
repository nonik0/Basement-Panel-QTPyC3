#pragma once
#include <Arduino.h>

class MatrixTaskHandler
{
protected:
    TaskHandle_t taskHandle = NULL;

public:
    virtual bool createTask() = 0;

    bool suspendTask()
    {
        if (taskHandle != NULL)
        {
            vTaskSuspend(taskHandle);
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