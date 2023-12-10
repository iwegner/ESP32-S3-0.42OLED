/*

  Webserver.ino
  This code creates a webserver for the ESP32-S3-0.42OLED board.
  The server  

  Using snippets from:
    * Universal 8bit Graphics Library (https://github.com/olikraus/u8g2/)
      Copyright (c) 2016, olikraus@gmail.com
      All rights reserved.
      Redistribution and use in source and binary forms, with or without modification, 
      are permitted provided that the following conditions are met:
      * Redistributions of source code must retain the above copyright notice, this list 
        of conditions and the following disclaimer.
      * Redistributions in binary form must reproduce the above copyright notice, this 
        list of conditions and the following disclaimer in the documentation and/or other 
        materials provided with the distribution.
      THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND 
      CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, 
      INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF 
      MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE 
      DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR 
      CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, 
      SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT 
      NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; 
      LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER 
      CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, 
      STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) 
      ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF 
      ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
    * "HelloServer" code from the ESP32
      Arduino Wifi examples, with some NeoPixels mixed in
      requires Adafruit NeoPixel library and the ESP32 libraries 
*/

// General OS
#include <Arduino.h>

// OLED
#include <U8g2lib.h>

// Wifi Server
#include <WiFi.h>
#include <WiFiClient.h>
#include <WebServer.h>
#include <ESPmDNS.h>

#include <NTPClient.h>
#include <WiFiUdp.h>


// adapt and modify for your SSID
//#include "private_wifi.h"
#include "private_wifi_dontcommit.h" // or create a file with that name with the contents of the file above

// Define NTP Client to get time
WiFiUDP ntpUDP;
NTPClient timeClient(ntpUDP);

#ifdef U8X8_HAVE_HW_SPI
  #include <SPI.h>
#endif

#ifdef U8X8_HAVE_HW_I2C
  #include <Wire.h>
#endif

// Defines for the OLED display
#define SDA_PIN 41
#define SCL_PIN 40

// Dimensions of the display
#define DISPLAY_MAX_X 71 // [0,71] -> 72 pixels
#define DISPLAY_MAX_Y 39 // [0,39] -> 40 pixels
#define DISPLAY_ROW_HIGHT 10
#define DISPLAY_MAX_CHAR_LINE 11

// Init the OLED display
U8G2_SSD1306_72X40_ER_F_HW_I2C u8g2(U8G2_R0, /* reset=*/ U8X8_PIN_NONE);   // EastRising 0.42" OLED

WebServer server(80); // Port for web server (80, default for HTTP)

// Received keys from web server
String received_string = "";

// Variable to save the current time
String timestamp;


// Create a simple HTML page, as a string showing
// some buttons to interact with the OLED display
char html[] = "<html><head><title>ESP32-S3-0.42OLED WebServer Demo</title>"
              "<meta name=\"viewport\" content=\"width=device-width initial-scale=1\">"
              "<style>html{font-family: Helvetica; display:inline-block; margin: 0px auto; text-align: center;} "
              "h1{color: #0F3376;}p{font-size: 1.3rem;}"
              ".button{display: inline-block; background-color: #000000; border: none; color: white; padding: 8px 30px; text-decoration: none; font-size: 20px; cursor: pointer;}"
              ".button2{background-color: #ff0000; padding: 8px 30px; font-size: 20px;}"
              ".button3{background-color: #36ba59; padding: 8px 30px; font-size: 20px;}"
              ".button4{background-color: #0000ff; padding: 8px 30px; font-size: 20px;}"
              "</style></head>"
              "<body><h1>ESP32-S3-0.42OLED WebServer Demo</h1>"
              "<p><a href=\"/O\"><button class=\"button\">led off</button></a></p>"
              "<p><a href=\"/R\"><button class=\"button2\">led red</button></a></p>"
              "<p><a href=\"/G\"><button class=\"button3\">led green</button></a></p>"
              "<p><a href=\"/B\"><button class=\"button4\">led blue</button></a></p>"
              "</body></html>";

void u8g2_prepare(void) {
  u8g2.setFont(u8g2_font_6x10_tf);
  u8g2.setFontRefHeightExtendedText();
  u8g2.setDrawColor(1);
  u8g2.setFontPosTop();
  u8g2.setFontDirection(0);
}


// Web server path handlers
void handleRoot() {
  server.sendHeader("Server", "Webserver Demo ESP32-S3-0.42OLED");
  server.sendHeader("Cache-Control", "no-cache");
  server.send(200, "text/html", html);
}

void handleOff() {
  received_string = "Off";
  handleRoot();
}

void handleRed() {
  received_string = "Red";
  handleRoot();
}

void handleGreen() {
  received_string = "Green";
  handleRoot();
}

void handleBlue() {
  received_string = "Blue";
  handleRoot();
}

void handleNotFound() {
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
  server.sendHeader("Server", "Webserver Demo ESP32-S3-0.42OLED");
  server.send(404, "text/plain", message);
}


/**
 * @brief Draw onto the OLED display.
 * Size see DISPLAY_MAX_X and DISPLAY_MAX_Y
 * x is |
 * y is ----
 * when USB-C port is on top
 * 
 * 
 */
void draw(void) {
  u8g2_prepare();
  
  // Draw a boarder
  u8g2.drawLine(0, 0, DISPLAY_MAX_X, 0);                          // |
  u8g2.drawLine(0, DISPLAY_MAX_Y, DISPLAY_MAX_X, DISPLAY_MAX_Y);  //    |
  u8g2.drawLine(0, 0, 0, DISPLAY_MAX_Y);                          // -
  u8g2.drawLine(DISPLAY_MAX_X, 0, DISPLAY_MAX_X, DISPLAY_MAX_Y);  // _

  // Draw headline
  u8g2.drawStr(0,0,"Webserver");
  // And an underline
  u8g2.drawLine(0, DISPLAY_ROW_HIGHT, DISPLAY_MAX_X, DISPLAY_ROW_HIGHT); // horizontal line underneath the headline

  // draw current color
  u8g2.drawStr(2, DISPLAY_MAX_Y * 1.0 / 3.0, received_string.c_str());

  // draw time in last line
  u8g2.drawStr(2, DISPLAY_MAX_Y * 2.0 / 3.0, timestamp.c_str());
}


void setup(void) {

  // Init OLED display
  Wire.begin(SDA_PIN, SCL_PIN);
  
  // Init display
  u8g2.begin();

  // Draw initial text on display
  u8g2.clearBuffer();
  u8g2_prepare();
  u8g2.drawStr(0,0,"Connecting");
  u8g2.sendBuffer();

  // Init wifi
  WiFi.mode(WIFI_STA);
  WiFi.begin(SSID, SSID_PASSWORD); // defined in included file

  // Wait for connection
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
  }

  // Print connection
  u8g2_uint_t line = 0;
  u8g2.clearBuffer();
  u8g2_prepare();
  u8g2.drawStr(0,line,"connected to");
  line += DISPLAY_ROW_HIGHT;
  u8g2.drawStr(0,line,SSID);
  line += DISPLAY_ROW_HIGHT;
  String ip_address = WiFi.localIP().toString();
  String ip_first = ip_address.substring(0, DISPLAY_MAX_CHAR_LINE);
  String ip_second = ip_address.substring(DISPLAY_MAX_CHAR_LINE, ip_address.length());
  u8g2.drawStr(0, line, ip_first.c_str());
  line += DISPLAY_ROW_HIGHT;
  u8g2.drawStr(0, line, ip_second.c_str());
  u8g2.sendBuffer();
  
  // Show for a while
  delay(3000);
  
  u8g2.clear();
  u8g2.sendBuffer();

  // Initialize a NTPClient to get time
  timeClient.begin();
  // Set offset time in seconds to adjust for your timezone, for example:
  // GMT +1 = 3600
  // GMT +8 = 28800
  // GMT -1 = -3600
  // GMT 0 = 0
  timeClient.setTimeOffset(3600);
  
  // Set hostname and advertise on the network (http://esp32oled.local)
  MDNS.begin("esp32oled");

    // specify how to handle different URL paths
  server.on("/", handleRoot);
  server.on("/O", handleOff);
  server.on("/R", handleRed);
  server.on("/G", handleGreen);
  server.on("/B", handleBlue);
  server.onNotFound(handleNotFound);
  server.begin();
}

void loop(void) {
  // update time
  timeClient.update();
  // while(!timeClient.update()) {
  //   timeClient.forceUpdate();
  // }
  
  // Formatted time
  timestamp = timeClient.getFormattedTime();

  // Display received event
  u8g2.clearBuffer();
  draw();
  u8g2.sendBuffer();

  server.handleClient();
  delay(2);

}
