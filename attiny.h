#pragma once

#include <Adafruit_seesaw.h>
#include <vector>

#include "matrix_16x9.h"
#include "task_handler.h"

#define SS_ATTINY_LED_PIN 10
#define SS_ATTINY_HUM_PIN 15
#define SS_ATTINY_GAS_PIN 16

// readings:
// humidity stable: 20-30
// humidity breath: ??
// gas stable: <240
// drunk gas: >300
// ~.25oz: 600?

using namespace std;

extern volatile bool display;
extern Matrix16x9TaskHandler matrix16x9TaskHandler;
TaskHandler* matrixTaskHandler = &matrix16x9TaskHandler;

class AttinyTaskHandler : public TaskHandler
{
private:
    const size_t AverageCount = 5;

    Adafruit_seesaw attinySs;
    vector<uint16_t> humidityReadings;
    vector<uint16_t> gasReadings;

    bool saveOverriddenMessage = false;
    const unsigned long MinOverrideTime = 10000;
    unsigned long messageOverriddenMillis = 0;
    uint16_t maxGasReading = 0;

public:
    AttinyTaskHandler() {}
    bool createTask() override;
    uint16_t getLastHumidityReading() { return humidityReadings.back(); }
    uint16_t getLastGasReading() { return gasReadings.back(); }
    uint16_t getWeightedHumidityReading();
    uint16_t getWeightedGasReading();

private:
    uint16_t readSensor(uint8_t pin, vector<uint16_t> &readings);
    void task(void *parameters) override;
};

bool AttinyTaskHandler::createTask()
{
    log_d("Starting ATtiny setup");

    if (_taskHandle != NULL)
    {
        log_w("Task already started");
        return false;
    }

    if (!attinySs.begin())
    {
        log_e("attiny seesaw not found!");
        return false;
    }

    attinySs.pinMode(SS_ATTINY_LED_PIN, OUTPUT);
    // analog setup not needed?

    xTaskCreate(taskWrapper, "AttinyTask", 4096, this, 10, &_taskHandle); // lower priority than matrix tasks

    log_d("Attiny setup complete");
    return true;
}

void AttinyTaskHandler::task(void *parameters)
{
    log_d("Starting AttinyTask");

    uint16_t humidityReading = 0;
    uint16_t gasReading = 0;
    while (1)
    {
        humidityReading = readSensor(SS_ATTINY_HUM_PIN, humidityReadings);
        delay(500);

        gasReading = readSensor(SS_ATTINY_GAS_PIN, gasReadings);
        delay(500);

        if (saveOverriddenMessage) {
            // override matrix message when gas detected
            if (getWeightedGasReading() > 300) // can optimize avg calculation by keeping a sum and subtracting the oldest reading
            {
                // first high reading, save existing message
                if (maxGasReading == 0) {
                    strncpy(_message, matrixTaskHandler->getMessage(), MaxMessageSize);
                }

                // update whenever reading increases
                if (gasReading > maxGasReading) {        
                    char drunkReading[MaxMessageSize];
                    snprintf(drunkReading, MaxMessageSize, "DRUNK:%d", gasReading); // TODO: calibrate and scale to BAC
                    matrixTaskHandler->setMessage(drunkReading);
                    maxGasReading = gasReading;
                    messageOverriddenMillis = millis();
                }
            }
            else if (maxGasReading > 0 && millis() - messageOverriddenMillis > MinOverrideTime)
            {
                matrixTaskHandler->setMessage(_message);
                maxGasReading = 0;
            }
        }
        else {
            // set gas reading as matrix message
            char gasReadingStr[MaxMessageSize];
            snprintf(gasReadingStr, MaxMessageSize, "%03d", gasReading);
            matrixTaskHandler->setMessage(gasReadingStr);
        }
    }
}

uint16_t AttinyTaskHandler::readSensor(uint8_t pin, vector<uint16_t> &readings)
{
    attinySs.digitalWrite(SS_ATTINY_LED_PIN, HIGH);

    uint16_t reading = attinySs.analogRead(pin);
    readings.push_back(reading);
    if (readings.size() > AverageCount)
    {
        readings.erase(readings.begin());
    }

    attinySs.digitalWrite(SS_ATTINY_LED_PIN, LOW);

    return reading;
}

uint16_t AttinyTaskHandler::getWeightedHumidityReading()
{
    uint32_t sum = 0;
    for (const auto &reading : humidityReadings)
    {
        sum += reading;
    }
    return sum / humidityReadings.size();
}

uint16_t AttinyTaskHandler::getWeightedGasReading()
{
    uint32_t sum = 0;
    for (uint16_t reading : gasReadings)
    {
        sum += reading;
    }
    return sum / gasReadings.size();
}