
#include <Adafruit_GFX.h>
#include <Adafruit_IS31FL3731.h>
#include <Adafruit_IS31FL3741.h>
#include <Adafruit_LEDBackpack.h>
#include <Adafruit_NeoPixel.h>
#include <Adafruit_NeoMatrix.h>
#include <Fonts/TomThumb.h>

#include <ArduinoOTA.h>
#include <ESPmDNS.h>
#include <WebServer.h>
#include <WiFi.h>

#include "secrets.h"

#define PIN A3

WebServer restServer(80);

TaskHandle_t matrix5x5TaskHandle = NULL;
TaskHandle_t matrix9x16TaskHandle = NULL;
TaskHandle_t matrix9x13TaskHandle = NULL;
TaskHandle_t matrix8x8TaskHandle = NULL;

//////////////////
Adafruit_NeoMatrix matrix5x5(5, 5, PIN,
                             NEO_MATRIX_BOTTOM + NEO_MATRIX_RIGHT +
                                 NEO_MATRIX_COLUMNS + NEO_MATRIX_PROGRESSIVE,
                             NEO_GRB + NEO_KHZ800);

char* matrix5x5Message;
const uint16_t colors[] = {
    matrix5x5.Color(255, 60, 0), matrix5x5.Color(255, 120, 0), matrix5x5.Color(255, 255, 0)};
uint16_t message_width;     // Computed in setup() below
int matrix5x5_x = matrix5x5.width();  // Start with message off right edge
int matrix5x5_y = matrix5x5.height(); // With custom fonts, y is the baseline, not top
int matrix5x5_pass = 0;               // Counts through the colors[] array
///////////////

///////////////
Adafruit_IS31FL3731 matrix9x16 = Adafruit_IS31FL3731();
uint8_t sweep[] = {1, 2, 3, 4, 6, 8, 10, 15, 20, 30, 40, 60, 60, 40, 30, 20, 15, 10, 8, 6, 4, 3, 2, 1};
///////////////

///////////////
Adafruit_IS31FL3741_QT matrix9x13 = Adafruit_IS31FL3741_QT();
char text[] = "Stella and Beau!"; // A message to scroll
int text_x = matrix9x13.width();  // Initial text position = off right edge
int text_y = 1;
int text_min; // Pos. where text resets (calc'd later)
uint16_t hue_offset = 0;
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

void Matrix5x5Task(void *parameters)
{
  while (1)
  {
    matrix5x5.fillScreen(0);   // Erase message in old position.
    matrix5x5.setCursor(matrix5x5_x, matrix5x5_y); // Set new cursor position,
    matrix5x5.print(matrix5x5Message);  // draw the message
    matrix5x5.show();          // and update the matrix.
    if (--matrix5x5_x < -message_width)
    {                        // Move 1 pixel left. Then, if scrolled off left...
      matrix5x5_x = matrix5x5.width(); // reset position off right edge and
      if (++matrix5x5_pass >= 3)
        matrix5x5_pass = 0; // increment color in list, rolling over if needed.
      matrix5x5.setTextColor(colors[matrix5x5_pass]);
    }
    delay(100); // 1/10 sec pause
  }
}

void Matrix5x5Setup()
{
  matrix5x5Message = new char[100];
  strcpy(matrix5x5Message, "BEAU IN TOW!");

  matrix5x5.begin();
  matrix5x5.setBrightness(40);       // Turn down brightness to about 15%
  matrix5x5.setFont(&TomThumb);      // Use custom font
  matrix5x5.setTextWrap(false);      // Allows text to scroll off edges
  matrix5x5.setTextColor(colors[0]); // Start with first color in list
  // To determine when the message has fully scrolled off the left side,
  // get the bounding rectangle of the text. As we only need the width
  // value, a couple of throwaway variables are passed to the bounds
  // function for the other values:
  int16_t d1;
  uint16_t d2;
  matrix5x5.getTextBounds(matrix5x5Message, 0, 0, &d1, &d1, &message_width, &d2);

  xTaskCreate(Matrix5x5Task, "Matrix5x5Task", 4096, NULL, 10, &matrix5x5TaskHandle);
}

void Matrix9x16Task(void *parameters)
{
  while (1)
  {
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
    Serial.println("IS31 not found");
    while (1)
      ;
  }
  Serial.println("IS31 found!");

  xTaskCreate(Matrix9x16Task, "Matrix9x16Task", 4096, NULL, 10, &matrix9x16TaskHandle);
}

void Matrix9x13Task(void *parameters)
{
  while (1)
  {
    matrix9x13.setCursor(text_x, text_y);
    for (int i = 0; i < (int)strlen(text); i++)
    {
      // set the color thru the rainbow
      uint32_t color888 = matrix9x13.ColorHSV(65536 * i / strlen(text));
      uint16_t color565 = matrix9x13.color565(color888);
      matrix9x13.setTextColor(color565, 0); // backound is '0' to erase previous text!
      matrix9x13.print(text[i]);            // write the letter
    }

    if (--text_x < text_min)
    {
      text_x = matrix9x13.width();
    }

    delay(25);

    // uint32_t i = 0;
    // for (int y = 0; y < matrix9x13.height(); y++)
    // {
    //   for (int x = 0; x < matrix9x13.width(); x++)
    //   {
    //     uint32_t color888 = matrix9x13.ColorHSV(i * 65536 / 117 + hue_offset);
    //     uint16_t color565 = matrix9x13.color565(color888);
    //     matrix9x13.drawPixel(x, y, color565);
    //     i++;
    //   }
    // }

    // hue_offset += 256;

    // matrix9x13.setGlobalCurrent(hue_offset / 256); // Demonstrate global current
  }
}

void Matrix9x13Setup()
{
  if (!matrix9x13.begin(IS3741_ADDR_DEFAULT))
  {
    Serial.println("IS41 not found");
    while (1)
      ;
  }
  Serial.println("IS41 found!");

  // Set brightness to max and bring controller out of shutdown state
  matrix9x13.setLEDscaling(0xFF);
  matrix9x13.setGlobalCurrent(0x10);
  matrix9x13.fill(0);
  matrix9x13.enable(true); // bring out of shutdown
  matrix9x13.setRotation(0);
  matrix9x13.setTextWrap(false);

  // Get text dimensions to determine X coord where scrolling resets
  uint16_t w, h;
  int16_t ignore;
  matrix9x13.getTextBounds(text, 0, 0, &ignore, &ignore, &w, &h);
  text_min = -w; // Off left edge this many pixels

  xTaskCreate(Matrix9x13Task, "Matrix9x13Task", 4096, NULL, 10, &matrix9x13TaskHandle);
}

void Matrix8x8Task(void *parameters)
{
  while (1)
  {
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
  matrix8x8.begin(0x70);
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
  ArduinoOTA.setPasswordHash("d5aa78e4cc65133dffb98c32605ff9d1"); // password

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
  //MDNS.addService("http", "tcp", 80);
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

void restSet5x5MatrixMessage()
{
  if (!restServer.hasArg("messasge"))
  {
    restServer.send(400, "text/plain", "No message provided");
    Serial.println("No message provided");
    return;
  }

  String message = restServer.arg("value");
  message = message.substring(0, 100);
  strcpy(matrix5x5Message, message.c_str());
}

void restSetup() {
  restServer.on("/", HTTP_GET, restIndex);
  restServer.on("/5x5", HTTP_GET, restSet5x5MatrixMessage);
  restServer.begin();

  Serial.println("REST server running");
}

void setup()
{
  Serial.begin(115200);
  Serial.println("Starting setup...");

  wifiSetup();
  mDnsSetup();
  otaSetup();
  restSetup();

  Wire.setClock(1000000);

  Matrix5x5Setup();
  Matrix9x16Setup();
  Matrix9x13Setup();
  Matrix8x8Setup();
}

void loop()
{
  // TOOD: wifi status?
  ArduinoOTA.handle();
  restServer.handleClient();
  checkWifiStatus();
}
