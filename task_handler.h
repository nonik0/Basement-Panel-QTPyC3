#pragma once
#include <Arduino.h>

class TaskHandler
{
protected:
    static const int MaxMessageSize = 100;

    TaskHandle_t _taskHandle = NULL;
    char _message[MaxMessageSize];

public:
    virtual bool createTask() = 0;

    char* getMessage() { return _message; }

    virtual void setMessage(const char *message) { strncpy(_message, message, MaxMessageSize); }

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
    virtual void task(void *parameters) = 0;

    static void taskWrapper(void *parameters)
    {
        TaskHandler *handler = static_cast<TaskHandler *>(parameters);
        handler->task(parameters);
    }
};