#pragma once

#include <Wire.h>
#include "display_task_handler.h"

class Nonik0TaskHandler : public DisplayTaskHandler
{
private:
    static const uint8_t I2C_ADDR = 0x13;

public:
    Nonik0TaskHandler() {}
    bool createTask() override;
    void setDisplay(bool state) override;
    void setMessage(const char *message) override;

private:
    void task(void *parameters) override;
};

bool Nonik0TaskHandler::createTask()
{
    log_d("Starting NONIK0 setup");

    if (_taskHandle != NULL)
    {
        log_w("Task already started");
        return false;
    }

    return true;
}

void Nonik0TaskHandler::setDisplay(bool displayState)
{
    DisplayTaskHandler::setDisplay(displayState);

    Wire.beginTransmission(I2C_ADDR);
    Wire.write((uint8_t)0x00);
    Wire.write((uint8_t)displayState);
    Wire.endTransmission();
}

void Nonik0TaskHandler::setMessage(const char *message)
{
    DisplayTaskHandler::setMessage(message);

    const size_t chunkSize = 31; // 32-byte, max chunk size for I2C transmission (-1 for end byte)
    size_t messageLength = strlen(message);
    for (size_t i = 0; i < messageLength; i += chunkSize)
    {
        Wire.beginTransmission(I2C_ADDR);
        Wire.write((uint8_t)0x01);

        size_t remaining = messageLength - i;
        size_t currentChunkSize = remaining > chunkSize ? chunkSize : remaining;

        Wire.write((const uint8_t *)(message + i), currentChunkSize);

        if (i + currentChunkSize >= messageLength)
        {
            Wire.write('\n');
        }

        Wire.endTransmission();
    }
}

void Nonik0TaskHandler::task(void *parameters)
{
}
