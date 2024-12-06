#pragma once

#include <Adafruit_seesaw.h>
#include <vector>

#include "display_task_handler.h"
#include "matrix_16x9.h"

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

extern Matrix16x9TaskHandler matrix16x9;
DisplayTaskHandler *matrixTaskHandler = &matrix16x9;

class AttinyTaskHandler : public DisplayTaskHandler
{
private:
    static const uint8_t I2C_ADDR = SEESAW_ADDRESS;
    static const uint8_t TASK_PRIORITY = 3; // lower priority than matrix tasks
    const size_t AverageCount = 5; // avg of last X readings
    const size_t MaxCount = 10;   // max of last X readings

    Adafruit_seesaw attinySs;
    vector<uint16_t> humidityReadings;
    vector<uint16_t> gasReadings;
    bool ledState = false;

    bool saveOverriddenMessage = false;
    const unsigned long MinOverrideTime = 10000;
    unsigned long messageOverriddenMillis = 0;

public:
    AttinyTaskHandler() {}
    bool createTask() override;
    uint16_t getLastHumidityReading() { return humidityReadings.back(); }
    uint16_t getLastGasReading() { return gasReadings.back(); }
    uint16_t getWeightedHumidityReading();
    uint16_t getMaxGasReading();

private:
    uint16_t readSensor(uint8_t pin, vector<uint16_t> &readings, size_t maxCount);
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

    if (!attinySs.begin(I2C_ADDR))
    {
        log_e("attiny seesaw not found!");
        return false;
    }

    attinySs.pinMode(SS_ATTINY_LED_PIN, OUTPUT);
    // analog setup not needed?

    xTaskCreate(taskWrapper, "AttinyTask", 4096, this, TASK_PRIORITY, &_taskHandle);

    log_d("Attiny setup complete");
    return true;
}

void AttinyTaskHandler::task(void *parameters)
{
    log_d("Starting AttinyTask");

    uint16_t humidityReading = 0;
    uint16_t gasReading = 0;
    uint16_t maxGasReading = 0;
    char gasReadingStr[MaxMessageSize];
    while (1)
    {
        humidityReading = readSensor(SS_ATTINY_HUM_PIN, humidityReadings, AverageCount);
        delay(500);


        gasReading = readSensor(SS_ATTINY_GAS_PIN, gasReadings, MaxCount);

        uint16_t newMaxGasReading = getMaxGasReading();
        if (newMaxGasReading != maxGasReading)
        {
            maxGasReading = newMaxGasReading;
            snprintf(gasReadingStr, MaxMessageSize, "%03d", getMaxGasReading());
            matrixTaskHandler->setMessage(gasReadingStr);
        }

        delay(500);

        // if (saveOverriddenMessage) {
        //     // override matrix message when gas detected
        //     if (getMaxGasReading() > 300) // can optimize avg calculation by keeping a sum and subtracting the oldest reading
        //     {
        //         // first high reading, save existing message
        //         if (maxGasReading == 0) {
        //             strncpy(_message, matrixTaskHandler->getMessage(), MaxMessageSize);
        //         }

        //         // update whenever reading increases
        //         if (gasReading > maxGasReading) {
        //             char drunkReading[MaxMessageSize];
        //             snprintf(drunkReading, MaxMessageSize, "DRUNK:%d", gasReading); // TODO: calibrate and scale to BAC
        //             matrixTaskHandler->setMessage(drunkReading);
        //             maxGasReading = gasReading;
        //             messageOverriddenMillis = millis();
        //         }
        //     }
        //     else if (maxGasReading > 0 && millis() - messageOverriddenMillis > MinOverrideTime)
        //     {
        //         matrixTaskHandler->setMessage(_message);
        //         maxGasReading = 0;
        //     }
        // }
        // else {
        // set gas reading as matrix message
        // char gasReadingStr[MaxMessageSize];
        // snprintf(gasReadingStr, MaxMessageSize, "%03d", gasReading);
        // matrixTaskHandler->setMessage(gasReadingStr);
        // }

        // led alternates with each loop
        ledState = !ledState;
    }
}

uint16_t AttinyTaskHandler::readSensor(uint8_t pin, vector<uint16_t> &readings, size_t maxCount)
{
    attinySs.digitalWrite(SS_ATTINY_LED_PIN, ledState);

    uint16_t reading = attinySs.analogRead(pin);
    readings.push_back(reading);
    while (readings.size() > maxCount)
    {
        readings.erase(readings.begin());
    }

    delay(100); // give time to see LED change
    attinySs.digitalWrite(SS_ATTINY_LED_PIN, !ledState);

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

uint16_t AttinyTaskHandler::getMaxGasReading()
{
    uint16_t max = 0;
    for (uint16_t reading : gasReadings)
    {
        if (reading > max)
        {
            max = reading;
        }
    }
    return max;
}

// TODO: still need REST endpoint?
//   void restSensors()
// {
//   String response = "{";
//   response += "\"humidityAvg\": " + String(attinyTaskHandler.getWeightedHumidityReading()) + ",";
//   response += "\"humidityRdg\": " + String(attinyTaskHandler.getLastHumidityReading()) + ",";
//   response += "\"gasMax\": " + String(attinyTaskHandler.getMaxGasReading()) + ",";
//   response += "\"gasRdg\": " + String(attinyTaskHandler.getLastGasReading());
//   response += "}";

//   restServer.send(200, "application/json", response);
// }