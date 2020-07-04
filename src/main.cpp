#include <NTPClient.h>
#include <ESP8266WiFi.h>
#include <ESP8266HTTPUpdateServer.h>
#include <ESP8266HTTPClient.h>
#include <ESP8266WebServer.h>
#include <ESP8266mDNS.h>
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

const char* update_path = "/firmware";
const char* update_username = OTA_USER;
const char* update_password = OTA_PASS;
const char *ssid = SSID_GENERAL;
const char *password = WIFI_PASSWORD;
const long utcOffsetInSeconds = 7200;

const char *invader1icon = "\x01";
const char *invader2icon = "\x02";
const char *clockicon = "\x03";
const char *clock2icon = "\x0A";
const char *celciusicon = "\x04";
const char *wifiicon = "\x0c";

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

int displayMode = 0;
char message[BUF_SIZE];

char temperatureBuffer[64];

WiFiUDP ntpUDP;
NTPClient timeClient(ntpUDP, "pool.ntp.org", utcOffsetInSeconds);
MD_Parola parolaClient = MD_Parola(HARDWARE_TYPE, CS_PIN, MAX_DEVICES);

ESP8266WebServer httpServer(80);
ESP8266HTTPUpdateServer httpUpdater;

void getDisplay() {
    if (httpServer.arg("mode") == "clock") {      
      parolaClient.displayClear();
      parolaClient.setFont(1, fontTinyNumbers);
      displayMode = 1;
    }

    if (httpServer.arg("mode") == "weather") {      
      parolaClient.displayClear();
      //Reset font to default
      parolaClient.setFont(1, nullptr);
      displayMode = 2;
    }
 
    httpServer.send(200, "text/json");
}

void setupWiFi() {
  WiFi.begin(ssid, password);

  parolaClient.displayZoneText(0, wifiicon, PA_LEFT, 0, 0, PA_PRINT, PA_NO_EFFECT);
  parolaClient.displayAnimate();

  //parolaClient.displayZoneText(0, wifiicon, PA_LEFT, 20, 10, PA_FADE, PA_FADE);  

  while ( WiFi.status() != WL_CONNECTED ) {
    // if(parolaClient.displayAnimate())
    //   if(parolaClient.getZoneStatus(0))
    //     parolaClient.displayReset();

    delay ( 500 );
    Serial.print ( "." );
  }

  Serial.println("");
  Serial.print("Connected to ");
  Serial.println(ssid);
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());

  httpUpdater.setup(&httpServer, update_path, update_username, update_password);
  //httpServer.begin(); > The actual server begin is in the Setup REST Service
  
  // WiFi.localIP().toString().toCharArray(message, 75);

  // parolaClient.displayClear();
  // parolaClient.displayZoneText(0, wifiicon, PA_LEFT, 0, 0, PA_PRINT, PA_NO_EFFECT);
  parolaClient.displayZoneText(1, "ok", PA_CENTER, 0, 0, PA_PRINT, PA_NO_EFFECT);
  parolaClient.displayAnimate();
}

void setupParola() {
  //Set 2 parola zones
  parolaClient.begin(2);
  parolaClient.setZone(0, 4, 4);
  parolaClient.setZone(1, 0, 3);

  parolaClient.setFont(0, fontIcons);
  //parolaClient.setFont(1, fontTinyNumbers);

  //parolaClient.displayAnimate();

  //parolaClient.setIntensity(0, 1);
}

void setupInternetTime() {
  timeClient.begin();
}

void setupRESTServiceRouting() {
    httpServer.on("/", HTTP_GET, []() {
        httpServer.send(200, F("text/html"),
            F("Welcome to the REST Web Server"));
    });

    httpServer.on(F("/display"), HTTP_GET, getDisplay);
}

void setupHandleNotFound() {
  String message = "File Not Found\n\n";
  message += "URI: ";
  message += httpServer.uri();
  message += "\nMethod: ";
  message += (httpServer.method() == HTTP_GET) ? "GET" : "POST";
  message += "\nArguments: ";
  message += httpServer.args();
  message += "\n";
  for (uint8_t i = 0; i < httpServer.args(); i++) {
    message += " " + httpServer.argName(i) + ": " + httpServer.arg(i) + "\n";
  }
  httpServer.send(404, "text/plain", message);
}

void setupRESTService() {
  // Set server routing
  setupRESTServiceRouting();
  // Set not found response
  httpServer.onNotFound(setupHandleNotFound);
  // Start server
  httpServer.begin();
}

void getOpenWeather() {
  Serial.println(openWeatherEndPoint);

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
    
    sprintf(temperatureBuffer, "%.02f", temperature);
    Serial.println(temperatureBuffer);

    // parolaClient.setFont(1, nullptr);
    // parolaClient.displayZoneText(1, buffer, PA_LEFT, 0, 0, PA_PRINT, PA_NO_EFFECT);
    // parolaClient.displayAnimate();
    // delay ( 10000 );
    // parolaClient.setFont(1, fontTinyNumbers);

    char buffer[64];

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

void displayClock() {  
  timeClient.update();
  timeClient.getFormattedTime().toCharArray(message, 75);
  
  parolaClient.displayZoneText(0, clock2icon, PA_LEFT, 0, 0, PA_PRINT, PA_NO_EFFECT);
  parolaClient.displayZoneText(1, message, PA_RIGHT, 0, 0, PA_PRINT, PA_NO_EFFECT);
  parolaClient.displayAnimate();

  delay(1000);  
}

void displayWeather() {
  //TODO: Glenn - We need to keep track of 'when' we retrieved our weather data and only refresh after a certain period to not go over our API limit  
  parolaClient.displayZoneText(0, celciusicon, PA_LEFT, 0, 0, PA_PRINT, PA_NO_EFFECT);  
  parolaClient.displayZoneText(1, temperatureBuffer, PA_CENTER, 0, 0, PA_PRINT, PA_NO_EFFECT);
  parolaClient.displayAnimate();
}

void handleDisplayMode() {
  switch (displayMode)
  {
    case 1:
      displayClock();
      break;
    case 2:
      displayWeather();
      break;
    default:
      break;
  }  
}

void setup()
{
  Serial.begin(115200);

  setupParola();

  setupWiFi();
  setupInternetTime();

  //TODO: Glenn - Remove delay ( only needed for Serial monitor viewing )
  delay ( 2500 );

  getOpenWeather();

  setupRESTService();
}

void loop()
{
  httpServer.handleClient();
  handleDisplayMode();
}