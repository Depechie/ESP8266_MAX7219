#include <NTPClient.h>
#include <ESP8266WiFi.h>
#include <WiFiUdp.h>
#include <MD_Parola.h>
#include <MD_MAX72xx.h>
// #include <SPI.h>

#include "fonts_data.h"

#define HARDWARE_TYPE MD_MAX72XX::GENERIC_HW
#define MAX_DEVICES 5

#define CLK_PIN   D5  // or SCK
#define DATA_PIN  D7  // or MOSI
#define CS_PIN    D4  // or SS

#define BUF_SIZE  75

const char *ssid = "Dep guests";
const char *password;
const long utcOffsetInSeconds = 7200;

const char *invader1icon = "\x01";
const char *invader2icon = "\x02";
const char *clockicon = "\x03";
const char *celciusicon = "\x04";

const char *tiny0 = "\x0A";
const char *tiny1 = "\x01";
const char *tiny2 = "\x02";
const char *tiny3 = "\x03";
const char *tiny4 = "\x04";
const char *tiny5 = "\x05";
const char *tiny6 = "\x06";
const char *tiny7 = "\x07";
const char *tiny8 = "\x08";
const char *tiny9 = "\x09";

char message[BUF_SIZE];

// char daysOfTheWeek[7][12] = {"Sunday", "Monday", "Tuesday", "Wednesday", "Thursday", "Friday", "Saturday"};

WiFiUDP ntpUDP;
NTPClient timeClient(ntpUDP, "pool.ntp.org", utcOffsetInSeconds);
MD_Parola parolaClient = MD_Parola(HARDWARE_TYPE, CS_PIN, MAX_DEVICES);

void setup()
{
  //Set 2 parola zones
  parolaClient.begin(2);
  parolaClient.setZone(0, 4, 4);
  parolaClient.setZone(1, 0, 3);

  //parolaClient.setIntensity(0, 1);

  Serial.begin(115200);

  WiFi.begin(ssid, password);

  while ( WiFi.status() != WL_CONNECTED ) {
    delay ( 500 );
    Serial.print ( "." );
  }

  timeClient.begin();

  parolaClient.setFont(0, fontIcons);
  parolaClient.setFont(1, fontTinyNumbers);

  parolaClient.displayZoneText(0, clockicon, PA_LEFT, 0, 0, PA_PRINT, PA_NO_EFFECT);
  parolaClient.displayAnimate();
}

void loop()
{
  timeClient.update();
  timeClient.getFormattedTime().toCharArray(message, 75);
  
  parolaClient.displayZoneText(1, message, PA_RIGHT, 0, 0, PA_PRINT, PA_NO_EFFECT);
  parolaClient.displayAnimate();

  delay(1000);
}