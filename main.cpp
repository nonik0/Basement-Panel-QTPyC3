
#include <Adafruit_GFX.h>
#include <Adafruit_IS31FL3731.h>
#include <Adafruit_IS31FL3741.h>
#include <Adafruit_LEDBackpack.h>
#include <Adafruit_NeoPixel.h>


#include <ArduinoOTA.h>
#include <ESPmDNS.h>
#include <WebServer.h>
#include <WiFi.h>

#include "secrets.h"
#include "matrix5x5.hpp"
#include "matrix9x13.hpp"

WebServer restServer(80);

TaskHandle_t matrix9x16TaskHandle = NULL;
TaskHandle_t matrix8x8TaskHandle = NULL;

volatile bool display = true;

///////////////
Adafruit_IS31FL3731 matrix9x16 = Adafruit_IS31FL3731();
uint8_t sweep[] = {1, 2, 3, 4, 6, 8, 10, 15, 20, 30, 40, 60, 60, 40, 30, 20, 15, 10, 8, 6, 4, 3, 2, 1};
///////////////

///////////////
#define LED_COUNT 64
#define REFRESHTIME 15
#define BASEDELAY 20

struct Pixel
{
  uint32_t color;
  int delay;
};
Pixel pixelData[LED_COUNT];

Adafruit_BicolorMatrix matrix8x8 = Adafruit_BicolorMatrix();
///////////////



void Matrix9x16Task(void *parameters)
{
  while (1)
  {
    if (!display)
    {
      matrix9x16.clear();
      delay(100);
      continue;
    }

    for (uint8_t incr = 0; incr < 24; incr++)
      for (uint8_t x = 0; x < 16; x++)
        for (uint8_t y = 0; y < 9; y++)
          matrix9x16.drawPixel(x, y, sweep[(x + y + incr) % 24]);
    delay(20);
  }
}

void Matrix9x16Setup()
{
  if (!matrix9x16.begin())
  {
    Serial.println("9x16 not found");
    return;
  }
  Serial.println("9x16 found!");

  xTaskCreate(Matrix9x16Task, "Matrix9x16Task", 4096, NULL, 10, &matrix9x16TaskHandle);
}

void Matrix8x8Task(void *parameters)
{
  while (1)
  {
    if (!display)
    {
      matrix8x8.fillScreen(LED_OFF);
      matrix8x8.writeDisplay();
      //matrix8x8.clear();
      delay(100);
      continue;
    }

    for (int i = 0; i < LED_COUNT; i++)
    {
      if (--pixelData[i].delay > 0)
        continue;

      pixelData[i].color = pixelData[i].color == LED_OFF ? LED_YELLOW : LED_OFF;
      pixelData[i].delay = random(BASEDELAY, BASEDELAY * 2);

      matrix8x8.drawPixel(i / 8, i % 8, pixelData[i].color);
    }

    matrix8x8.writeDisplay();
    delay(REFRESHTIME);
  }
}

void Matrix8x8Setup()
{
  if (!matrix8x8.begin(0x70))
  {
    Serial.println("8x8 not found");
    return;
  }
  Serial.println("8x8 found!");

  matrix8x8.setBrightness(5);

  randomSeed(analogRead(0));

  for (int i = 0; i < LED_COUNT; i++)
  {
    pixelData[i].color = LED_YELLOW;
    pixelData[i].delay = random(BASEDELAY * 5, BASEDELAY * 10);

    matrix8x8.drawPixel(i / 8, i % 8, pixelData[i].color);
  }

  xTaskCreate(Matrix8x8Task, "Matrix8x8Task", 4096, NULL, 10, &matrix8x8TaskHandle);
}

void otaSetup()
{
  Serial.println("OTA setting up...");

  // ArduinoOTA.setPort(3232);
  ArduinoOTA.setHostname("I2C-LED-Controller");
  // ArduinoOTA.setPasswordHash("d5aa78e4cc65133dffb98c32605ff9d1"); // password

  ArduinoOTA
      .onStart([]()
               {
        String type;
        if (ArduinoOTA.getCommand() == U_FLASH)
          type = "sketch";
        else  // U_SPIFFS
          type = "filesystem";

        Serial.println("Start updating " + type); })
      .onEnd([]()
             { Serial.println("\nEnd"); })
      .onProgress([](unsigned int progress, unsigned int total)
                  { Serial.printf("Progress: %u%%\r", (progress / (total / 100))); })
      .onError([](ota_error_t error)
               {
        Serial.printf("Error[%u]: ", error);
        if (error == OTA_AUTH_ERROR)
          Serial.println("Auth Failed");
        else if (error == OTA_BEGIN_ERROR)
          Serial.println("Begin Failed");
        else if (error == OTA_CONNECT_ERROR)
          Serial.println("Connect Failed");
        else if (error == OTA_RECEIVE_ERROR)
          Serial.println("Receive Failed");
        else if (error == OTA_END_ERROR)
          Serial.println("End Failed"); });
  ArduinoOTA.begin();

  Serial.println("OTA setup complete.");
}

void mDnsSetup()
{
  if (!MDNS.begin("i2c-led-controller"))
  {
    Serial.println("Error setting up MDNS responder!");

    return;
  }
  Serial.println("mDNS responder started");

  // TODO: OTA service?
  // MDNS.addService("http", "tcp", 80);
}

void wifiSetup()
{
  Serial.println("Wifi setting up...");
  WiFi.mode(WIFI_STA);
  WiFi.begin(WIFI_SSID, WIFI_PASS);
  while (WiFi.waitForConnectResult() != WL_CONNECTED)
  {
    Serial.println("Connection Failed! Rebooting...");
    delay(5000);
    ESP.restart();
  }

  Serial.print("Wifi ready, IP address: ");
  Serial.println(WiFi.localIP());
}

volatile long wifiStatusDelayMs = 0;
int wifiDisconnects = 0;
void checkWifiStatus()
{
  if (wifiStatusDelayMs < 0)
  {
    try
    {
      if (WiFi.status() != WL_CONNECTED)
      {
        Serial.println("Reconnecting to WiFi...");
        WiFi.disconnect();
        WiFi.reconnect();
        wifiDisconnects++;
        Serial.println("Reconnected to WiFi");
      }
    }
    catch (const std::exception &e)
    {
      Serial.println("Wifi error:" + String(e.what()));
      wifiStatusDelayMs = 10 * 60 * 1000; // 10 minutes
    }

    wifiStatusDelayMs = 60 * 1000; // 1 minute
  }
}

void restIndex()
{
  Serial.println("Serving index.html");
  restServer.send(200, "text/plain", "test");
  Serial.println("Served index.html");
}

void restDisplay()
{
  if (restServer.hasArg("plain")) {
    String body = restServer.arg("plain");
    body.toLowerCase();

    if (body == "off") {
      Serial.println("Turning display off");
      display = false;
    } else if (body == "on") {
      Serial.println("Turning display on");
      display = true;
    } else {
      restServer.send(400, "text/plain", body);
      return;
    }
  }

  restServer.send(200, "text/plain", display ? "on" : "off");
}

void restSetMessage(char *curMessage)
{
  if (!restServer.hasArg("message"))
  {
    restServer.send(400, "text/plain", "No message provided");
    Serial.println("No message provided");
    return;
  }

  String newMessage = restServer.arg("message");
  newMessage = newMessage.substring(0, 100);
  strcpy(curMessage, newMessage.c_str());

  restServer.send(200, "text/plain", curMessage);
}

void restSetup()
{
  restServer.on("/", HTTP_GET, restIndex);
  restServer.on("/display", restDisplay);
  restServer.on("/5x5", HTTP_GET, []()
                { restSetMessage(matrix5x5Message); }); // TODO: support GET or POST params?
  restServer.on("/9x13", HTTP_GET, []()
                { restSetMessage(matrix9x13Message); });
  restServer.begin();

  Serial.println("REST server running");
}

void setup()
{
  delay(5000);

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
}
