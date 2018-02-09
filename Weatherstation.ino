#include <M5Stack.h>

#include <SPIFFS.h>
#include <SPI.h>

// Additional UI functions
#include "GfxUi.h"

// #include "Font.h"

// Download helper
#include "WebResource.h"

#include <ArduinoOTA.h>
//#include <DNSServer.h>

// Helps with connecting to internet
//#include <WiFiManager.h>

// check settings.h for adapting to your needs
#include "settings.h"
#include <JsonListener.h>
#include <WundergroundClient.h>
#include "TimeClient.h"

// HOSTNAME for OTA update
#define HOSTNAME "M5Stack-OTA-Weatherstation"

/*****************************
   Important: see settings.h to configure your settings!!!
 * ***************************/


GfxUi ui = GfxUi();

int place;

WebResource webResource;
TimeClient timeClient(UTC_OFFSET);

// Set to false, if you prefere imperial/inches, Fahrenheit
WundergroundClient wunderground(IS_METRIC);

//declaring prototypes
//void configModeCallback (WiFiManager *myWiFiManager);
void downloadCallback(String filename, int16_t bytesDownloaded, int16_t bytesTotal);
ProgressCallback _downloadCallback = downloadCallback;
void downloadResources();
void updateData();
void drawProgress(uint8_t percentage, String text);
void screenPrint(int x, int y, String text, uint16_t color);
void drawTime();
void drawCurrentWeather();
void drawForecast();
void drawForecastDetail(uint16_t x, uint16_t y, uint8_t dayIndex);
String getMeteoconIcon(String iconText);
void drawAstronomy();
void drawSeparator(uint16_t y);
bool readyForWeatherUpdate = true;
long lastDownloadUpdate = millis();

int fc = 1;

void setup() {
  M5.begin();
  Serial.begin(115200);
  M5.Lcd.setRotation(2);
  M5.Lcd.fillScreen(BLACK);
//  M5.Lcd.setFont(&Roboto_Bold_6);
  ui.setTextAlignment(CENTER);
  screenPrint(10, 16, "Connecting to WiFi", ORANGE); //, "CENTER");


  // OTA Setup
  String hostname(HOSTNAME);
//  hostname += String(ESP.getEfuseMac(), HEX);
 // WiFi.hostname(hostname);
  ArduinoOTA.setHostname((const char *)hostname.c_str());
  ArduinoOTA.begin();
  SPIFFS.begin();

  //Uncomment if you want to update all internet resources
  //SPIFFS.format();

  // download images from the net. If images already exist don't download
  downloadResources();

  // load the weather information
  updateData();
}

long lastDrew = 0;
void loop() {
  // Handle OTA update requests
  ArduinoOTA.handle();

  // Check if we should update the clock
  if (millis() - lastDrew > 30000 && wunderground.getSeconds() == "00") {
    // drawTime();
    lastDrew = millis();
  }

  // Check if we should update weather information
  if (millis() - lastDownloadUpdate > 1000 * UPDATE_INTERVAL_SECS) {
    updateData();
    lastDownloadUpdate = millis();
  }
  if (fc == 1)
  {
    fc = 0 ;
    drawCurrentWeather();
    delay(10000);
  }
  else if (fc == 0 )
  {
    fc = 1;
    drawForecast();
    delay(10000);
  }
}

//// Called if WiFi has not been configured yet
//void configModeCallback (WiFiManager *myWiFiManager) {
//  ui.setTextAlignment(CENTER);
//  screenPrint(4, 28, "wifi Manager", ORANGE);
//  screenPrint(4, 42, "PleasE Connect to AP", ORANGE);
//  screenPrint(4, 56, myWiFiManager->getConfigPortalSSID(), WHITE);
//  screenPrint(4, 70, "To set up WiFi Configuration", ORANGE);
//}

// callback called during download of files. Updates progress bar
void downloadCallback(String filename, int16_t bytesDownloaded, int16_t bytesTotal) {
  Serial.println(String(bytesDownloaded) + " / " + String(bytesTotal));

  int percentage = 100 * bytesDownloaded / bytesTotal;
  if (percentage == 0) {
    screenPrint(12, 16, filename, GREEN);
    // M5.Lcd.print(12, 16, filename);
  }
  if (percentage % 5 == 0) {
    ui.setTextAlignment(CENTER);
    M5.Lcd.setTextColor(ORANGE, BLACK);
    M5.Lcd.print(12, 16, String(percentage) + "%");
    screenPrint(12,16, "100 %", ORANGE);
    ui.drawProgressBar(10, 118, 128 - 20, 15, percentage, WHITE, BLUE);
  }

}

// Download the bitmaps
void downloadResources() {
  M5.Lcd.fillScreen(BLACK);;
  char id[5];
  for (int i = 0; i < 21; i++) {
    sprintf(id, "%02d", i);
    M5.Lcd.fillRect(0, 128, 128, 20, BLACK);
    webResource.downloadFile("http://www.squix.org/blog/wunderground/" + wundergroundIcons[i] + ".bmp", wundergroundIcons[i] + ".bmp", _downloadCallback);
  }
  for (int i = 0; i < 21; i++) {
    sprintf(id, "%02d", i);
    M5.Lcd.fillRect(0, 128, 128, 20, BLACK);
    webResource.downloadFile("http://www.squix.org/blog/wunderground/mini/" + wundergroundIcons[i] + ".bmp", "/mini/" + wundergroundIcons[i] + ".bmp", _downloadCallback);
  }
  for (int i = 0; i < 24; i++) {
    M5.Lcd.fillRect(0, 128, 128, 20, BLACK);
    webResource.downloadFile("http://www.squix.org/blog/moonphase_L" + String(i) + ".bmp", "/moon" + String(i) + ".bmp", _downloadCallback);
  }
}

// Update the internet based information and update screen
void updateData() {
  if (readyForWeatherUpdate == true)
  {
    M5.Lcd.fillScreen(BLACK);
    drawProgress(20, "Updating time...");
    timeClient.updateTime();
    drawProgress(50, "Updating conditions...");
    wunderground.updateConditions(WUNDERGRROUND_API_KEY, WUNDERGRROUND_LANGUAGE, WUNDERGROUND_COUNTRY, WUNDERGROUND_CITY);
    drawProgress(70, "Updating forecasts...");
    wunderground.updateForecast(WUNDERGRROUND_API_KEY, WUNDERGRROUND_LANGUAGE, WUNDERGROUND_COUNTRY, WUNDERGROUND_CITY);
    drawProgress(90, "Updating astronomy...");
    wunderground.updateAstronomy(WUNDERGRROUND_API_KEY, WUNDERGRROUND_LANGUAGE, WUNDERGROUND_COUNTRY, WUNDERGROUND_CITY);
    //lastUpdate = timeClient.getFormattedTime();
    readyForWeatherUpdate = false;
    drawProgress(100, "Done...");
    delay(1000);
    M5.Lcd.fillScreen(BLACK);
    drawTime();

  }
  else if (readyForWeatherUpdate == false) {
    timeClient.updateTime();
    wunderground.updateConditions(WUNDERGRROUND_API_KEY, WUNDERGRROUND_LANGUAGE, WUNDERGROUND_COUNTRY, WUNDERGROUND_CITY);
    delay(100);
    wunderground.updateForecast(WUNDERGRROUND_API_KEY, WUNDERGRROUND_LANGUAGE, WUNDERGROUND_COUNTRY, WUNDERGROUND_CITY);
    delay(100);
    wunderground.updateAstronomy(WUNDERGRROUND_API_KEY, WUNDERGRROUND_LANGUAGE, WUNDERGROUND_COUNTRY, WUNDERGROUND_CITY);
    delay(100);
    drawTime();
  }

    drawAstronomy();
}

// Progress bar helper
void drawProgress(uint8_t percentage, String text) {
  ui.setTextAlignment(CENTER);
  M5.Lcd.fillRect(0, 0, 128, 60, BLACK);
  screenPrint(12, 16, text, ORANGE);
  ui.drawProgressBar(10, 120, 120 - 20, 15, percentage, WHITE, LIGHTBLUE);
}

// draws the clock
void drawTime() {
  
  M5.Lcd.fillRect(0,0,128,35, BLACK);
  String date = wunderground.getDate();
  screenPrint(12, 10, date, WHITE);
  String time = timeClient.getHours() + ":" + timeClient.getMinutes();
  screenPrint(12, 25,"Oppdatert: " + time, WHITE);
  drawSeparator(35);
}

// draws current weather information
void drawCurrentWeather() {

  // Weather Icon
  M5.Lcd.fillRect(0, 38, 128, 60, BLACK);
  String weatherIcon = getMeteoconIcon(wunderground.getTodayIcon());
  ui.drawBmp("/mini/" + weatherIcon + ".bmp", 5, 50);

  // Weather Text
  String text = wunderground.getWeatherText();
  Serial.println(text.length());
  screenPrint(20, 40, text, ORANGE);
  ui.setTextAlignment(RIGHT);
  String degreeSign = "F";
  if (IS_METRIC) {
    degreeSign = "C";
  }
  String temp = wunderground.getCurrentTemp() + degreeSign;
  M5.Lcd.setTextSize(2);
  screenPrint(62, 65, temp, WHITE);
  M5.Lcd.setTextSize(1);

}

// draws the three forecast columns
void drawForecast() {
  M5.Lcd.fillRect(0, 38, 128, 60, BLACK);
  drawForecastDetail(5, 40, 2);
  drawForecastDetail(60, 40, 4);
  //  drawForecastDetail(60, 30, 4);
}

// helper for the forecast columns
void drawForecastDetail(uint16_t x, uint16_t y, uint8_t dayIndex) {

  String day = wunderground.getForecastTitle(dayIndex).substring(0, 3);
  day.toUpperCase();
  screenPrint(x + 23 , y - 2, day, ORANGE);
  String weatherIcon = getMeteoconIcon(wunderground.getForecastIcon(dayIndex));
  ui.drawBmp("/mini/" + weatherIcon + ".bmp", x + 5, y + 10);
  screenPrint(x + 15 , y + 7, wunderground.getForecastLowTemp(dayIndex) + "|" + wunderground.getForecastHighTemp(dayIndex), WHITE);

}

// draw moonphase and sunrise/set and moonrise/set
void drawAstronomy() {
  drawSeparator(99);
  //   M5.Lcd.fillRect(0, 100, 128, 70, BLACK);
  int moonAgeImage = 24 * wunderground.getMoonAge().toInt() / 30.0;
// int moonAgeImage = wunderground.getMoonAge().toInt() - 1;
  ui.drawBmp("/moon" + String(moonAgeImage) + ".bmp", 5, 100);
  Serial.println(wunderground.getMoonAge().toInt());
  Serial.println( moonAgeImage);
  screenPrint(75, 105, "Sol opp", YELLOW);
  screenPrint(75, 115, wunderground.getSunriseTime(), WHITE);
  screenPrint(75, 130, "Sol ned", YELLOW);
  screenPrint(75, 140, wunderground.getSunsetTime(), WHITE);
}

// Helper function, should be part of the weather station library and should disappear soon
String getMeteoconIcon(String iconText) {
  if (iconText == "F") return "chanceflurries";
  if (iconText == "Q") return "chancerain";
  if (iconText == "W") return "chancesleet";
  if (iconText == "V") return "chancesnow";
  if (iconText == "S") return "chancetstorms";
  if (iconText == "B") return "clear";
  if (iconText == "Y") return "cloudy";
  if (iconText == "F") return "flurries";
  if (iconText == "M") return "fog";
  if (iconText == "E") return "hazy";
  if (iconText == "Y") return "mostlycloudy";
  if (iconText == "H") return "mostlysunny";
  if (iconText == "H") return "partlycloudy";
  if (iconText == "J") return "partlysunny";
  if (iconText == "W") return "sleet";
  if (iconText == "R") return "rain";
  if (iconText == "W") return "snow";
  if (iconText == "B") return "sunny";
  if (iconText == "0") return "tstorms";

  return "unknown";
}

// if you want separators, uncomment the M5.Lcd-line
void drawSeparator(uint16_t y) {
  M5.Lcd.drawFastHLine(10, y, 128 - 2 * 10, 0x4228);
}

void screenPrint(int x, int y, String text, uint16_t color) //, String alignment)
{
  M5.Lcd.setCursor(x, y);
  M5.Lcd.setTextWrap(false);
  M5.Lcd.setTextColor(color);
  M5.Lcd.print(text);
}
