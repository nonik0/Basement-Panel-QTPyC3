#pragma once

#include <Adafruit_LEDBackpack.h>
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
    static const uint8_t I2C_ADDR_BARGRAPH = 0x72;
    static const uint8_t TASK_PRIORITY = 3; // lower priority than matrix tasks
    const size_t AverageCount = 5;          // avg of last X readings
    const size_t MaxCount = 10;             // max of last X readings
    const size_t BargraphCount = 24;

    Adafruit_seesaw attinySs;
    Adafruit_24bargraph bargraph;
    vector<uint16_t> humidityReadings;
    vector<uint16_t> gasReadings;
    bool ledState = false;
    bool bargraphInit = false;

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
    void breathalyzerAnimation();

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

    if (bargraph.begin(I2C_ADDR_BARGRAPH))
    {
        bargraph.begin(I2C_ADDR_BARGRAPH);
        bargraph.setBrightness(15); //[0-15]
        bargraphInit = true;
    }
    else
    {
        log_e("bargraph not found");
    }

    // analog setup not needed?

    xTaskCreate(taskWrapper, "AttinyTask", 4096, this, TASK_PRIORITY, &_taskHandle);

    log_d("Attiny setup complete");
    return true;
}

void AttinyTaskHandler::task(void *parameters)
{
    log_d("Starting AttinyTask");

    bool animation = false;
    uint16_t humidityReading = 0;
    uint16_t avgHumidityReading = 0;
    uint16_t gasReading = 0;
    uint16_t maxGasReading = 0;
    uint8_t bargraphHeight = 0;
    unsigned long bargraphLastUpdateMillis = 0;
    char readingStr[MaxMessageSize];
    while (1)
    {
        bool showHumidity = millis() % 10000 < 5000;

        humidityReading = readSensor(SS_ATTINY_HUM_PIN, humidityReadings, AverageCount);
        uint16_t newAvgHumidityReading = getWeightedHumidityReading();
        if (newAvgHumidityReading != avgHumidityReading && showHumidity)
        {
            avgHumidityReading = newAvgHumidityReading;
            snprintf(readingStr, MaxMessageSize, "%03d", getWeightedHumidityReading());
            matrixTaskHandler->setMessage(readingStr);

            if (bargraphInit && !animation && avgHumidityReading > 30) {
                breathalyzerAnimation();
            }
        }
        delay(500);

        gasReading = readSensor(SS_ATTINY_GAS_PIN, gasReadings, MaxCount);
        uint16_t newMaxGasReading = getMaxGasReading();
        if (newMaxGasReading != maxGasReading)
        {
            if (!showHumidity)
            {
                maxGasReading = newMaxGasReading;
                snprintf(readingStr, MaxMessageSize, "%03d", getMaxGasReading());
                matrixTaskHandler->setMessage(readingStr);
            }

            if (bargraphInit)
            {
                uint8_t color = LED_OFF;
                uint8_t height = 0;
                if (maxGasReading < 200)
                {
                    color = LED_GREEN;
                    height = map(maxGasReading, 0, 200, 1, BargraphCount);
                }
                else if (maxGasReading < 400)
                {
                    color = LED_YELLOW;
                    height = map(maxGasReading, 200, 400, 1, BargraphCount);
                }
                else if (maxGasReading < 600)
                {
                    color = LED_RED;
                    height = map(maxGasReading, 400, 600, 1, BargraphCount);
                }
                else
                {
                    // TODO
                }

                if (millis() - bargraphLastUpdateMillis > 500)
                {
                    bargraphLastUpdateMillis = millis();
                    if (bargraphHeight < height)
                    {
                        bargraphHeight++;
                    }
                    else if (bargraphHeight > height)
                    {
                        bargraphHeight--;
                    }

                    for (int i = 0; i < BargraphCount; i++)
                    {
                        bargraph.setBar(BargraphCount - i - 1, i < bargraphHeight ? color : LED_OFF);
                    }
                    bargraph.writeDisplay();
                }
            }
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

void AttinyTaskHandler::breathalyzerAnimation()
{
    const int DrunkReadingThreshold = 600;

    bargraph.clear();
    bargraph.writeDisplay();

    // TODO
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