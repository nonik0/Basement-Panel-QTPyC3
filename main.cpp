#include "matrix8x8.hpp"

#include "matrix_5x5.h"
#include "matrix_8x8M.h"
#include "matrix_13x9.h"
#include "matrix_16x9.h"
#include "services.hpp"

volatile bool display = true;
Matrix5x5TaskHandler matrix5x5TaskHandler;
Matrix8x8MTaskHandler matrix8x8MTaskHandler;
Matrix13x9TaskHandler matrix13x9TaskHandler;
Matrix16x9TaskHandler matrix16x9TaskHandler;

void setup()
{
  delay(5000);
  Wire.begin();
  Wire.setClock(400000UL);

  Serial.begin(115200);
  log_d("Starting setup...");

  wifiSetup();
  mDnsSetup();
  otaSetup();
  restSetup();

  Matrix8x8Setup();

  matrix5x5TaskHandler.createTask();
  matrix8x8MTaskHandler.createTask();
  matrix13x9TaskHandler.createTask();
  matrix16x9TaskHandler.createTask();

  log_d("Setup complete");
}

void loop()
{
  ArduinoOTA.handle();
  restServer.handleClient();
  checkWifiStatus();
  delay(10);
}
