#include <NTPClient.h>
#include <ESP8266WiFi.h>
#include <WiFiUdp.h>
#include <MD_Parola.h>
#include <MD_MAX72xx.h>
// #include <SPI.h>

#include "fonts_data.h"

#define HARDWARE_TYPE MD_MAX72XX::GENERIC_HW
#define MAX_DEVICES 1 //5

#define CLK_PIN   D5  // or SCK
#define DATA_PIN  D7  // or MOSI
#define CS_PIN    D4  // or SS

#define BUF_SIZE  75

const char *ssid = "Dep guests";
const char *password;
const long utcOffsetInSeconds = 7200;
const char *invader1 = "\x01";
const char *invader2 = "\x02";

char message[BUF_SIZE];

// char daysOfTheWeek[7][12] = {"Sunday", "Monday", "Tuesday", "Wednesday", "Thursday", "Friday", "Saturday"};

WiFiUDP ntpUDP;
NTPClient timeClient(ntpUDP, "pool.ntp.org", utcOffsetInSeconds);
MD_Parola parolaClient = MD_Parola(HARDWARE_TYPE, CS_PIN, MAX_DEVICES);

void setup()
{
  parolaClient.begin();
  //parolaClient.setIntensity(0, 1);

  Serial.begin(115200);

  WiFi.begin(ssid, password);

  while ( WiFi.status() != WL_CONNECTED ) {
    delay ( 500 );
    Serial.print ( "." );
  }

  timeClient.begin();

  parolaClient.setFont(fontInvaders);
  parolaClient.displayText(invader2, PA_LEFT, 0, 0, PA_PRINT, PA_NO_EFFECT);
  parolaClient.displayAnimate();

  delay(5000);

  parolaClient.setFont(nullptr);
}

void loop()
{
  timeClient.update();
  timeClient.getFormattedTime().toCharArray(message, 75);
  
  parolaClient.displayText(message, PA_LEFT, 0, 0, PA_PRINT, PA_NO_EFFECT);
  parolaClient.displayAnimate();

  delay(1000);
}