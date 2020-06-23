#include <NTPClient.h>
#include <ESP8266WiFi.h>
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

const char *ssid = SSID_GENERAL;
const char *password = WIFI_PASSWORD;
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

int displayMode = 0;
// **

char message[BUF_SIZE];

// char daysOfTheWeek[7][12] = {"Sunday", "Monday", "Tuesday", "Wednesday", "Thursday", "Friday", "Saturday"};

WiFiUDP ntpUDP;
NTPClient timeClient(ntpUDP, "pool.ntp.org", utcOffsetInSeconds);
MD_Parola parolaClient = MD_Parola(HARDWARE_TYPE, CS_PIN, MAX_DEVICES);

ESP8266WebServer server(80);

void getDisplay() {
    if (server.arg("mode") == "clock") {
      parolaClient.displayClear();
      displayMode = 1;
    }

    if (server.arg("mode") == "weather") {
      parolaClient.displayClear();
      displayMode = 2;
    }
 
    server.send(200, "text/json");
}

void setupWiFi() {
  WiFi.begin(ssid, password);

  while ( WiFi.status() != WL_CONNECTED ) {
    delay ( 500 );
    Serial.print ( "." );
  }
  Serial.println("");
  Serial.print("Connected to ");
  Serial.println(ssid);
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());  
}

void setupParola() {
  //Set 2 parola zones
  parolaClient.begin(2);
  parolaClient.setZone(0, 4, 4);
  parolaClient.setZone(1, 0, 3);

  parolaClient.setFont(0, fontIcons);
  parolaClient.setFont(1, fontTinyNumbers);

  parolaClient.displayAnimate();

  //parolaClient.setIntensity(0, 1);
}

void setupInternetTime() {
  timeClient.begin();
}

void setupRESTServiceRouting() {
    server.on("/", HTTP_GET, []() {
        server.send(200, F("text/html"),
            F("Welcome to the REST Web Server"));
    });

    server.on(F("/display"), HTTP_GET, getDisplay);
}

void setupHandleNotFound() {
  String message = "File Not Found\n\n";
  message += "URI: ";
  message += server.uri();
  message += "\nMethod: ";
  message += (server.method() == HTTP_GET) ? "GET" : "POST";
  message += "\nArguments: ";
  message += server.args();
  message += "\n";
  for (uint8_t i = 0; i < server.args(); i++) {
    message += " " + server.argName(i) + ": " + server.arg(i) + "\n";
  }
  server.send(404, "text/plain", message);
}

void setupRESTService() {
  // Set server routing
  setupRESTServiceRouting();
  // Set not found response
  server.onNotFound(setupHandleNotFound);
  // Start server
  server.begin();
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

void displayClock() {  
  timeClient.update();
  timeClient.getFormattedTime().toCharArray(message, 75);
  
  parolaClient.displayZoneText(0, clockicon, PA_LEFT, 0, 0, PA_PRINT, PA_NO_EFFECT);
  parolaClient.displayZoneText(1, message, PA_RIGHT, 0, 0, PA_PRINT, PA_NO_EFFECT);
  parolaClient.displayAnimate();

  delay(1000);  
}

void displayWeather() {

  parolaClient.displayZoneText(0, celciusicon, PA_LEFT, 0, 0, PA_PRINT, PA_NO_EFFECT);  
  //parolaClient.displayZoneText(1, message, PA_RIGHT, 0, 0, PA_PRINT, PA_NO_EFFECT);
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

  // getOpenWeather();

  setupRESTService();
}

void loop()
{
  server.handleClient();
  handleDisplayMode();
}