#pragma once

#include <Adafruit_LEDBackpack.h>
#include <Adafruit_seesaw.h>
#include <vector>

#include "display_task_handler.h"
#include "matrix_16x9.h"

#define SS_ATTINY_LED_PIN 10
#define SS_ATTINY_HUM_PIN 15
#define SS_ATTINY_GAS_PIN 16

using namespace std;

extern Matrix16x9TaskHandler matrix16x9;
DisplayTaskHandler *matrixTaskHandler = &matrix16x9;

class BreathalyzerTaskHandler : public DisplayTaskHandler
{
private:
    static const uint8_t I2C_ADDR = SEESAW_ADDRESS;
    static const uint8_t I2C_ADDR_BARGRAPH = 0x72;
    static const uint8_t TASK_PRIORITY = 3; // lower priority than matrix tasks
    const size_t AverageCount = 5;          // avg of last X readings
    const size_t MaxCount = 10;             // max of last X readings
    const size_t BargraphCount = 24;

    const uint16_t BreathHumidityReadingThreshold = 35;
    const uint16_t StableGasReading = 200;
    const uint16_t TipsyGasReadingThreshold = 500;
    const uint16_t DrunkGasReadingThreshold = 700;
    const uint16_t SaturationGasReading = 800;
    const uint8_t TipsyBarHeight = map(TipsyGasReadingThreshold, StableGasReading, SaturationGasReading, 0, BargraphCount);
    const uint8_t DrunkBarHeight = map(DrunkGasReadingThreshold, StableGasReading, SaturationGasReading, 0, BargraphCount);

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
    BreathalyzerTaskHandler() {}
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

bool BreathalyzerTaskHandler::createTask()
{
    log_d("Starting Breathalyzer setup");

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

    log_d("Breathalyzer setup complete");
    return true;
}

void BreathalyzerTaskHandler::task(void *parameters)
{
    log_d("Starting BreathalyzerTask");

    bool animation = false;
    uint16_t humidityReading = 0;
    uint16_t avgHumidityReading = 0;
    uint16_t gasReading = 0;
    uint16_t maxGasReading = 0;
    uint8_t bargraphHeight = 0;
    unsigned long bargraphLastUpdateMillis = millis();
    char readingStr[MaxMessageSize];
    while (1)
    {
        humidityReading = readSensor(SS_ATTINY_HUM_PIN, humidityReadings, AverageCount);
        uint16_t newAvgHumidityReading = getWeightedHumidityReading();
        bool showHumidity = newAvgHumidityReading > BreathHumidityReadingThreshold && millis() % 10000 < 5000;
        if (newAvgHumidityReading != avgHumidityReading)
        {
            avgHumidityReading = newAvgHumidityReading;

            if (showHumidity)
            {
                snprintf(readingStr, MaxMessageSize, "%03d", getWeightedHumidityReading());
                matrixTaskHandler->setMessage(readingStr);
            }

            if (bargraphInit)
            {
                if (avgHumidityReading > BreathHumidityReadingThreshold && !animation)
                {
                    animation = true;
                    breathalyzerAnimation();
                }
                // humidity level needs to fall below threshold again to reinitiate animation
                else if (animation && avgHumidityReading < BreathHumidityReadingThreshold)
                {
                    animation = false;
                }
            }
        }
        delay(500);

        gasReading = readSensor(SS_ATTINY_GAS_PIN, gasReadings, MaxCount);
        uint16_t newMaxGasReading = getMaxGasReading();
        if (newMaxGasReading != maxGasReading)
        {
            maxGasReading = newMaxGasReading;

            if (!showHumidity)
            {
                snprintf(readingStr, MaxMessageSize, "%03d", maxGasReading);
                matrixTaskHandler->setMessage(readingStr);
            }

            if (bargraphInit)
            {

                if (millis() - bargraphLastUpdateMillis > 500)
                {
                    bargraphLastUpdateMillis = millis();
                    bargraph.clear();

                    if (_display)
                    {
                        uint8_t gasReadingHeight = map(maxGasReading, StableGasReading, SaturationGasReading, 0, BargraphCount);
                        if (bargraphHeight < gasReadingHeight)
                        {
                            bargraphHeight++;
                        }
                        else if (bargraphHeight > gasReadingHeight)
                        {
                            bargraphHeight--;
                        }

                        for (int i = 0; i < BargraphCount; i++)
                        {
                            // felt like one-lining this
                            bargraph.setBar(BargraphCount - i - 1, (i < bargraphHeight) ? LED_YELLOW : ((i == TipsyBarHeight) ? LED_GREEN : (i == DrunkBarHeight ? LED_RED : LED_OFF)));
                        }
                    }
                    bargraph.writeDisplay();
                }
            }
        }

        delay(500);

        // led alternates with each loop
        ledState = !ledState;
    }
}

uint16_t BreathalyzerTaskHandler::readSensor(uint8_t pin, vector<uint16_t> &readings, size_t maxCount)
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

uint16_t BreathalyzerTaskHandler::getWeightedHumidityReading()
{
    uint32_t sum = 0;
    for (const auto &reading : humidityReadings)
    {
        sum += reading;
    }
    return sum / humidityReadings.size();
}

uint16_t BreathalyzerTaskHandler::getMaxGasReading()
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

void BreathalyzerTaskHandler::breathalyzerAnimation()
{
    bargraph.clear();
    bargraph.writeDisplay();

    // quick animation to show breath detected
    for (int i = BargraphCount - 1; i >= 0; i--)
    {
        bargraph.setBar(i, LED_YELLOW);
        bargraph.writeDisplay();
        delay(50);
    }
    for (int i = 0; i < BargraphCount; i++)
    {
        bargraph.setBar(i, LED_OFF);
        bargraph.writeDisplay();
        delay(50);
    }

    uint8_t barHeight = 0;
    uint16_t maxGasReading = getMaxGasReading();
    unsigned long lastIncreaseMillis = millis();
    while (millis() - lastIncreaseMillis < 5000)
    {
        // keep reading sensor for max gas reading
        uint16_t gasReading = readSensor(SS_ATTINY_GAS_PIN, gasReadings, MaxCount);
        uint16_t newMaxGasReading = getMaxGasReading();
        if (newMaxGasReading > maxGasReading)
        {
            maxGasReading = newMaxGasReading;
        }

        uint8_t gasReadingBarHeight = map(maxGasReading, StableGasReading, SaturationGasReading, 0, BargraphCount);
        if (gasReadingBarHeight > barHeight)
        {
            uint8_t color = LED_GREEN;
            if (maxGasReading > TipsyGasReadingThreshold)
            {
                color = LED_YELLOW;
            }
            else if (maxGasReading > DrunkGasReadingThreshold)
            {
                color = LED_RED;
            }
            bargraph.setBar(BargraphCount - barHeight - 1, color);
            barHeight++;
            bargraph.writeDisplay();
            lastIncreaseMillis = millis();
        }

        delay(500);
    }
}
