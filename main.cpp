#include "matrix5x5.hpp"
#include "matrix8x8.hpp"
#include "matrix9x13.hpp"
#include "matrix9x16.hpp"
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
  Matrix9x16Setup();
  Matrix9x13Setup();
  Matrix8x8Setup();
}

void loop()
{
  ArduinoOTA.handle();
  restServer.handleClient();
  checkWifiStatus();
  delay(10);
}
