
// #include "breathalyzer.h"
#include "matrix_5x5.h"
#include "matrix_8x8.h"
#include "matrix_8x8M.h"
#include "matrix_13x9.h"
#include "matrix_16x9.h"
#include "nonik0.h"
#include "wifi_services.h"

// BreathalyzerTaskHandler breathalyzer;
Matrix5x5TaskHandler matrix5x5;
Matrix8x8TaskHandler matrix8x8;
Matrix8x8MTaskHandler matrix8x8M;
Matrix13x9TaskHandler matrix13x9;
Matrix16x9TaskHandler matrix16x9;
Nonik0TaskHandler nonik0;
WifiServices wifiServices;

void setup()
{
  delay(5000);
  Wire.begin();
  Wire.setClock(400000UL);

  Serial.begin(115200);
  log_d("Starting setup...");

  wifiServices.setup(DEVICE_NAME);

  // breathalyzer.createTask();
  matrix5x5.createTask();
  matrix8x8.createTask();
  matrix8x8M.createTask();
  matrix13x9.createTask();
  matrix16x9.createTask();
  wifiServices.createTask();

  // wifiServices.registerSetDisplayCallback([&](bool state)
  //                                         { breathalyzer.setDisplay(state); });
  wifiServices.registerSetDisplayCallback([&](bool state)
                                          { matrix5x5.setDisplay(state); });
  wifiServices.registerSetDisplayCallback([&](bool state)
                                          { matrix8x8.setDisplay(state); });
  wifiServices.registerSetDisplayCallback([&](bool state)
                                          { matrix8x8M.setDisplay(state); });
  wifiServices.registerSetDisplayCallback([&](bool state)
                                          { matrix13x9.setDisplay(state); });
  wifiServices.registerSetDisplayCallback([&](bool state)
                                          { matrix16x9.setDisplay(state); });
  wifiServices.registerSetDisplayCallback([&](bool state)
                                          { nonik0.setDisplay(state); });

  wifiServices.registerSetMessageCallback("/5x5", [](const char *message)
                                          { if (strlen(message) > 0)  matrix5x5.setMessage(message);
                                            return matrix5x5.getMessage(); });
  wifiServices.registerSetMessageCallback("/8x8", [](const char *message)
                                          { if (strlen(message) > 0)  matrix8x8.setMessage(message);
                                            return matrix8x8.getMessage(); });
  wifiServices.registerSetMessageCallback("/8x8M", [](const char *message)
                                          { if (strlen(message) > 0)  matrix8x8M.setMessage(message);
                                            return matrix8x8M.getMessage(); });
  wifiServices.registerSetMessageCallback("/13x9", [](const char *message)
                                          { if (strlen(message) > 0)  matrix13x9.setMessage(message);
                                            return matrix13x9.getMessage(); });
  wifiServices.registerSetMessageCallback("/16x9", [](const char *message)
                                          { if (strlen(message) > 0)  matrix16x9.setMessage(message);
                                            return matrix16x9.getMessage(); });
  wifiServices.registerSetMessageCallback("/nonik0", [](const char *message)
                                          { if (strlen(message) > 0)  nonik0.setMessage(message);
                                            return nonik0.getMessage(); });

  log_d("Setup complete");
}

void loop()
{
}
