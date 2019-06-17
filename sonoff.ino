/*
  Copyright (C) 2019  Alexi Nasra
  This program is free software you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation, either version 3 of the License, or
  (at your option) any later version.
  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.
  You should have received a copy of the GNU General Public License
  along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#include <ESP8266WiFi.h>
#include <WiFiClient.h>
#include <ESP8266WebServer.h>
void blink(int duration, int repeat);
void led_on();
void led_off();
void relay_on();
void relay_off();
void relay_toggle();
bool isConfigMode();
void setConfigMode(bool mode);

// Pin config
#define BUTTON_SONOFF 0
#define RELAY_SONOFF 12
#define LED_SONOFF 13


// Blink duration
#define BLINK_DURATION = 2000;

static bool config_mode = false;
static bool sonoff_on = false;

// webserver for restful operations
ESP8266WebServer server(80);

void setup()
{
  
  Serial.begin(115200);
  blink(500,5);
//  WiFiManager.resetSettings();
//  ESP.eraseConfig();
//  ESP.reset();
//  ESP.restart();
  // initialise digital pin BUTTON_SONOFF as an input.
  pinMode(BUTTON_SONOFF, INPUT);
  
  // initialise digital pin RELAY_SONOFF as an output.
  pinMode(RELAY_SONOFF, OUTPUT);
  digitalWrite(RELAY_SONOFF, LOW);
  // initialise digital pin LED_SONOFF as an output.
  pinMode(LED_SONOFF, OUTPUT);
  digitalWrite(LED_SONOFF, HIGH);
  
  Serial.println();
  
  //start power on signal.
  blink(300,3);
  

  if (WiFi.SSID() == "") {
    //WiFi.mode(WIFI_AP_STA);
    //WiFi.beginSmartConfig();
    //while (!WiFi.smartConfigDone()) {
    //  blink(5, 1000);
    //  delay(2000);
    //}
    IPAddress local_IP(192,168,4,22);
    IPAddress gateway(192,168,4,9);
    IPAddress subnet(255,255,255,0);
    if(WiFi.softAPConfig(local_IP, gateway, subnet)) {
      WiFi.softAP("ESPsoftAP_02");
      
      Serial.print("Soft-AP IP address = ");
      Serial.println(WiFi.softAPIP());
      setConfigMode(true);
    } else {
      while(true) {
        blink(100, 2);
        delay(1000);
      }
    }
    
  } else {
    Serial.print("Reusing saved SSID: %s ");
    Serial.println(WiFi.SSID());
    
    WiFi.begin();
    Serial.print("Connecting");
    while (WiFi.status() != WL_CONNECTED)
    {
      blink(100, 1);
      Serial.print(".");
    }
    Serial.println();

    blink(2000, 1);
    Serial.print("Connected, IP address: ");
    Serial.println(WiFi.localIP());
  }

  if (isConfigMode()) {
    server.on("/config", HTTP_POST, []() {
      if (server.hasArg("ssid")) {
        WiFi.begin(server.arg("ssid"), server.arg("password"));
        Serial.print("Connecting");
        while (WiFi.status() != WL_CONNECTED)
        {
          blink(100, 1);
          Serial.print(".");
        }
        Serial.println();
    
        blink(2000, 1);
        Serial.print("Connected, IP address: ");
        Serial.println(WiFi.localIP());
      } else {
         server.sendHeader("Location", "/",true);   //Redirect to our html web page  
         server.send(302, "text/plane","");
      }
    });  
  }

  server.on("/", [](){
    String webPage = "<h1>SONOFF Web Server</h1><p><a href=\"on\"><button>ON</button></a>&nbsp;<a href=\"off\"><button>OFF</button></a></p>";  
    if (isConfigMode()){
      webPage += "<form action=\"/config\" method=\"POST\">";
      webPage +=  "<dl>";
      webPage +=    "<dt>";
      webPage +=      "SSID:";
      webPage +=    "</dt>";
      webPage +=    "<dd>";
      webPage +=      "<input type=\"text\" name=\"ssid\">";
      webPage +=    "</dd>";
      webPage +=    "<dt>";
      webPage +=      "Password:";
      webPage +=    "</dt>";
      webPage +=    "<dd>";
      webPage +=      "<input type=\"text\" name=\"password\">";
      webPage +=    "</dd>";
      webPage +=    "<dd>";
      webPage +=      "<input type=\"submit\" value=\"Setup\">";
      webPage +=    "</dd>";
      webPage +=  "</dl>";
      webPage += "</form>";
    }
    server.send(200, "text/html", webPage);
  });
  server.on("/on", []() {
    relay_on();
    String webPage = "{\"status\": \"ON\"}";  
    server.send(200, "text/json", webPage);
  });
  server.on("/off", []() {
    relay_off();
    String webPage = "{\"status\": \"OFF\"}"; 
    server.send(200, "text/json", webPage);
  });
  server.on("/toggle", []() {
    relay_toggle();
    String webPage = sonoff_on ? "{\"status\": \"ON\"}" : "{\"status\": \"OFF\"}"; 
    server.send(200, "text/json", webPage);
  });
  server.on("/status", []() {
    String webPage = sonoff_on ? "{\"status\": \"ON\"}" : "{\"status\": \"OFF\"}"; 
    server.send(200, "text/json", webPage);
  });
  server.begin();
}

void loop() {
  int val = digitalRead(BUTTON_SONOFF);
  if (val == LOW) {
    relay_toggle();
  }
  server.handleClient();
}

void blink(int duration, int repeat){
  int i = 0;

  if (sonoff_on) {
    led_off();
    delay(100);
  }
  do {
    led_on();
    delay(duration / 2);
    led_off();
    delay(duration / 2);
  } while((++i) < repeat);
  
  if (sonoff_on) {
    delay(100);
    led_on();
  }
}

void led_on() {
  digitalWrite(LED_SONOFF, LOW); // LOW will turn on the LED
}
void led_off() {
  digitalWrite(LED_SONOFF, HIGH); // HIGH will turn on the LED
}

void relay_on() {
  sonoff_on = true;
  led_on();
  digitalWrite(RELAY_SONOFF, HIGH); // LOW will turn on the LED
}

void relay_off() {
  sonoff_on = false;
  led_off();
  digitalWrite(RELAY_SONOFF, LOW); // HIGH will turn on the LED
}

void relay_toggle() {
  if (sonoff_on) {
    relay_off();
  } else {
    relay_on();
  }
}

bool isConfigMode() {
  return config_mode;
}

void setConfigMode(bool mode) {
  config_mode = mode;
}
