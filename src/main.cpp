#include <NTPClient.h>
#include <ESP8266WiFi.h>
#include <ESP8266HTTPClient.h>
#include <WiFiUdp.h>
#include <ArduinoJson.h>
#include <MD_Parola.h>
#include <MD_MAX72xx.h>

#include "arduino_secrets.h"
#include "fonts_data.h"

#define HARDWARE_TYPE MD_MAX72XX::GENERIC_HW
#define MAX_DEVICES 5

#define CLK_PIN   D5  // or SCK
#define DATA_PIN  D7  // or MOSI
#define CS_PIN    D4  // or SS

#define BUF_SIZE  75

const char *ssid = SSID_GUESTS;
const char *password;
const long utcOffsetInSeconds = 7200;

const char *invader1icon = "\x01";
const char *invader2icon = "\x02";
const char *clockicon = "\x03";
const char *celciusicon = "\x04";

// ** OpenWeather
const char* host = "https://api.openweathermap.org";
const char* url = "/data/2.5/weather";
const char* openweathermapid = OPENWEATHER_API_ID;
const char* openweathermapq = OPENWEATHER_API_LOCATION;
const char* openweathermapunits = "metric"; //So that OpenWeather will return Celsius
const char* fingerprint = OPENWEATHER_API_FINGERPRINT;
//Determining capacipy is done by pasting JSON data into the input part of this site https://arduinojson.org/v6/assistant/
const size_t capacity = JSON_ARRAY_SIZE(1) + JSON_OBJECT_SIZE(1) + 2*JSON_OBJECT_SIZE(2) + JSON_OBJECT_SIZE(4) + JSON_OBJECT_SIZE(5) + JSON_OBJECT_SIZE(6) + JSON_OBJECT_SIZE(13) + 280;

String openWeatherEndPoint = String(host) + String(url) +
               "?q=" + openweathermapq +
               "&units=" + openweathermapunits +
               "&APPID=" + openweathermapid;

String line;
// **

char message[BUF_SIZE];

// char daysOfTheWeek[7][12] = {"Sunday", "Monday", "Tuesday", "Wednesday", "Thursday", "Friday", "Saturday"};

WiFiUDP ntpUDP;
NTPClient timeClient(ntpUDP, "pool.ntp.org", utcOffsetInSeconds);
MD_Parola parolaClient = MD_Parola(HARDWARE_TYPE, CS_PIN, MAX_DEVICES);

void setup()
{
  Serial.begin(115200);

  //Set 2 parola zones
  parolaClient.begin(2);
  parolaClient.setZone(0, 4, 4);
  parolaClient.setZone(1, 0, 3);

  parolaClient.setFont(0, fontIcons);
  parolaClient.setFont(1, fontTinyNumbers);

  parolaClient.displayZoneText(0, clockicon, PA_LEFT, 0, 0, PA_PRINT, PA_NO_EFFECT);
  parolaClient.displayAnimate();

  //parolaClient.setIntensity(0, 1);

  WiFi.begin(ssid, password);

  while ( WiFi.status() != WL_CONNECTED ) {
    delay ( 500 );
    Serial.print ( "." );
  }

  timeClient.begin();

  Serial.println(openWeatherEndPoint);

  //TODO: Glenn - Remove delay ( only needed for Serial monitor viewing )
  delay ( 2500 );

  WiFiClientSecure wifiClient;
  HTTPClient httpClient;

  //You need to set the website fingerprint to enable HTTPS - https://github.com/esp8266/Arduino/blob/master/libraries/ESP8266HTTPClient/examples/BasicHttpsClient/BasicHttpsClient.ino
  //SHA1 fingerprint can be found by clicking on the Lock icon in a webbrowser when visiting the site in question
  wifiClient.setFingerprint(fingerprint);
  httpClient.begin(wifiClient, openWeatherEndPoint);

  int httpCode = httpClient.GET();
  // Serial.println(String(httpCode));

  if(httpCode == HTTP_CODE_OK)
  {
    String payload = httpClient.getString();

    // Serial.println(payload);

    DynamicJsonDocument doc(capacity);
    deserializeJson(doc, payload);

    JsonObject openWeatherDataMain = doc["main"];
    JsonObject OpenWeatherDataWeather = doc["weather"][0];
    JsonObject openWeatherDataWind = doc["wind"];

    double temperature = openWeatherDataMain["temp"];
    char buffer[64];
    sprintf(buffer, "Temp: %.02f", temperature);
    Serial.println(buffer);

    // parolaClient.setFont(1, nullptr);
    // parolaClient.displayZoneText(1, buffer, PA_LEFT, 0, 0, PA_PRINT, PA_NO_EFFECT);
    // parolaClient.displayAnimate();
    // delay ( 10000 );
    // parolaClient.setFont(1, fontTinyNumbers);

    int humidity = openWeatherDataMain["humidity"];
    sprintf(buffer, "Humidity: %d", humidity);
    Serial.println(buffer);

    int pressure = openWeatherDataMain["pressure"];
    sprintf(buffer, "Pressure: %d", pressure);
    Serial.println(buffer);

    double wind = openWeatherDataWind["speed"];
    sprintf(buffer, "Wind speed: %.02f m/s", wind);
    Serial.println(buffer);

    const char* weather = OpenWeatherDataWeather["main"];
    const char* description = OpenWeatherDataWeather["description"];
    sprintf(buffer, "Weather: %s (%s)", weather, description);
    Serial.println(buffer);
  }

  httpClient.end();
}

void loop()
{
  timeClient.update();
  timeClient.getFormattedTime().toCharArray(message, 75);
  
  parolaClient.displayZoneText(1, message, PA_RIGHT, 0, 0, PA_PRINT, PA_NO_EFFECT);
  parolaClient.displayAnimate();

  delay(1000);
}