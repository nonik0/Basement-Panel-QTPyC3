#include <ArduinoOTA.h>
#include <ESPmDNS.h>
#include <WebServer.h>
#include <WiFi.h>

#include "secrets.h"

WebServer restServer(80);
extern volatile bool display;
extern char *matrix5x5Message;
extern char *matrix13x9Message;

void wifiSetup()
{
  Serial.println("Wifi setting up...");

  WiFi.mode(WIFI_STA);
  WiFi.begin(WIFI_SSID, WIFI_PASS);
  WiFi.setTxPower(WIFI_POWER_8_5dBm); // seems to improve connection stability when issues on default tx power
  Serial.print("Wifi TX: ");
  Serial.println(WiFi.getTxPower());

  while (WiFi.waitForConnectResult() != WL_CONNECTED)
  {
    Serial.println("Connection Failed! Rebooting...");
    delay(5000);
    ESP.restart();
  }

  Serial.print("Wifi ready, IP address: ");
  Serial.println(WiFi.localIP());
}

volatile unsigned long wifiLastCheckMillis = 0;
volatile int wifiStatusDelayMs = 5000;
int wifiDisconnects = 0;
void checkWifiStatus()
{
  if (millis() > wifiLastCheckMillis + wifiStatusDelayMs)
  {
    log_v("Wifi TX: ");
    log_v(WiFi.getTxPower());

    try
    {
      if (WiFi.status() != WL_CONNECTED)
      {
        log_w("Reconnecting to WiFi...");
        WiFi.disconnect();
        WiFi.reconnect();
        wifiDisconnects++;
        log_w("Reconnected to WiFi");
      }
    }
    catch (const std::exception &e)
    {
      log_e("Wifi error: %s", String(e.what()));
      wifiStatusDelayMs = 10 * 60 * 1000; // 10 minutes
    }

    wifiLastCheckMillis = millis();
    wifiStatusDelayMs = 60 * 1000; // 1 minute
  }
}

void mDnsSetup()
{
  if (!MDNS.begin("i2c-led-controller"))
  {
    log_e("Error setting up MDNS responder!");
    return;
  }

  log_d("mDNS responder started");
}

void otaSetup()
{
  Serial.println("OTA setting up...");

  ArduinoOTA.setHostname("I2C-LED-Controller");

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

void restIndex()
{
  Serial.println("Serving index.html");
  restServer.send(200, "text/plain", "test");
  Serial.println("Served index.html");
}

void restDisplay()
{
  if (restServer.hasArg("plain"))
  {
    String body = restServer.arg("plain");
    body.toLowerCase();

    if (body == "off")
    {
      Serial.println("Turning display off");
      display = false;
    }
    else if (body == "on")
    {
      Serial.println("Turning display on");
      display = true;
    }
    else
    {
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
    log_w("No message provided");
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
                { restSetMessage(matrix5x5Message); });
  restServer.on("/13x9", HTTP_GET, []()
                { restSetMessage(matrix5x5Message); });
                //{ restSetMessage(matrix13x9Message); });
  restServer.begin();

  log_d("REST server running");
}