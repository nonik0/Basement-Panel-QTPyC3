#include "matrix5x5.hpp"
#include "matrix8x8.hpp"
#include "matrix13x9.hpp"
#include "matrix16x9.hpp"
#include "services.hpp"

volatile bool display = true;

void setup()
{
  Wire.begin();
  Wire.setClock(400000UL);

  Serial.begin(115200);
  Serial.println("Starting setup...");

  wifiSetup();
  mDnsSetup();
  otaSetup();
  restSetup();

  Matrix5x5Setup();
  Matrix8x8Setup();
  Matrix13x9Setup();
  Matrix16x9Setup();
}

void loop()
{
  ArduinoOTA.handle();
  restServer.handleClient();
  checkWifiStatus();
  delay(10);
}
