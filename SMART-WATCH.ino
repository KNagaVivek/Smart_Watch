#include <driver/rtc_io.h>
#include <WiFi.h>
#include "time.h"
#include "SSD1306.h"
#include <Wire.h>
#include <TinyGPS++.h>
#include "twilio.hpp"
#include "ThingSpeak.h";

#define BUTTON_PIN 19
#define TS_Channel_ID 1679754
#define TS_API "ZCAZXFTOSJJO6MU9"


SSD1306 display(0x3c, 21, 22);
int c = 0;
int ls = LOW;  // the previous state from the input pin
int cs;     // the current reading from the input pin

WiFiClient client;


static const char *ssid = "V2037";
static const char *password = "lokesh@123";

//static const char *account_sid = "AC6296e127859b6735be7bb5a45ad2a5a6";
static const char *account_sid = "AC1a3e0e5fdb382fc7adb6444a2b370bd8";
//static const char *auth_token = "66de627e23592b3d0d92751fb313f710";
static const char *auth_token = "a3be44ebba590dfc1afbc26c709b1f67";
// Phone number should start with "+<countrycode>"
//static const char *from_number = "+19107668616";
static const char *from_number = "+16603337536";

// You choose!
// Phone number should start with "+1910"
static const char *to_number = "+919160401792";
String message = "Sent from ESP32:\n";

const char* ntpServer = "pool.ntp.org";
const long gmtOffset_sec = 19800;
const int  daylightOffset_sec = 0;


#define RXD2 16
#define TXD2 17
HardwareSerial neogps(1);

TinyGPSPlus gps;

Twilio *twilio;


void printLocalTime()
{
  struct tm timeinfo;
  if(!getLocalTime(&timeinfo)) {
    Serial.println("Failed to obtain time");
    return;
  }

  int h = timeinfo.tm_hour;
  int m = timeinfo.tm_min;
  int d = timeinfo.tm_mday;
  int mn = timeinfo.tm_mon;
  int y = timeinfo.tm_year;
  mn = mn+1;
  y = 1900+y;

  static char hr[15];
  static char minn[15];
  static char dayy[15];
  static char mont[15];
  static char yr[15];

  dtostrf(h,2, 0, hr);
  dtostrf(m,2, 0, minn);
  dtostrf(d,2, 0, dayy);
  dtostrf(mn,2, 0, mont);
  dtostrf(y,2, 0, yr);

  //Serial.println(&timeinfo, "%A, %B %d %Y %H:%M:%S");
  display.clear();
  //display.setRotation(2); 
  display.setFont(ArialMT_Plain_24);
  display.drawString(26,10,hr);
  display.drawString(59,10,":");
  display.drawString(72,10,minn);

  display.setFont(ArialMT_Plain_10);
  display.drawString(30,36,dayy);
  display.drawString(44,36,mont);
  display.drawString(62,36,yr);
  display.display();
  delay(2000);
}


void setup() {
  Serial.begin(115200);
  display.init();
  Serial.printf("Connecting to %s ",ssid);
  WiFi.begin(ssid,password);
  while(WiFi.status() != WL_CONNECTED) {
    c++;
    delay(500);
    display.print(".");
    display.clear();
    display.drawString(0,0,"Connecting.....");
    display.display();
    display.clear();
    if(c>=100) {
      ESP.restart();
    }
  }
  Serial.println("CONNECTED");
  
  pinMode(BUTTON_PIN, INPUT_PULLUP);
  
  twilio = new Twilio(account_sid, auth_token);
  
  display.clear();
  display.drawString(0,24,"Synchronizing the clock");
  display.drawString(0,48,"Using NTF");
  display.display();
  delay(1500);

  configTime(gmtOffset_sec, daylightOffset_sec, ntpServer);
  //printLocalTime();

  neogps.begin(9600, SERIAL_8N1, RXD2, TXD2);
  ThingSpeak.begin(client);
  
  delay(1000);
  
  //WiFi.disconnect(true);
    //WiFi.mode(WIFI_OFF);
  //Serial.println("Going to sleeep now");
  //Serial.flush();
  //esp_deep_sleep_start();

}

void loop() {
  cs=digitalRead(BUTTON_PIN);
  if(ls==HIGH && cs==LOW) {
    boolean newData = false;
      for (unsigned long start = millis(); millis() - start < 1000;)
  {
    while (neogps.available())
    {
      if (gps.encode(neogps.read()))
      {
        newData = true;
      }
    }
  }

  if(newData == true) {
    newData = false;
    Serial.println(gps.satellites.value());
    print_speed();
  } else {
    Serial.println("No Data");
    display.clear();
    display.setFont(ArialMT_Plain_24);
    display.drawString(0,24,"No Data");
    display.display();
    delay(2000);
  }  
 } 
 else {
  printLocalTime(); 
 }
 ls=cs;
  
}

  
void print_speed() {
  if (gps.location.isValid() == 1) {
    Serial.print("Lat: ");
    Serial.println(gps.location.lat(),6);

    Serial.print("Lng: ");
    Serial.println(gps.location.lng(),6);

    Serial.print("Speed: ");
    Serial.println(gps.speed.kmph());
    
    Serial.print("SAT:");
    Serial.println(gps.satellites.value());

    Serial.print("ALT:");
    Serial.println(gps.altitude.meters(), 0);

    char lati[10],loni[10];
    dtostrf(gps.location.lat(),2,6,lati);
    dtostrf(gps.location.lng(),2,6,loni);
    
    message += "I am in trouble pls help me. I am in this location: https://www.google.com/maps/?q=";
    message +=String(lati)+","+String(loni)+"\n";
    String response;
    bool success = twilio->send_message(to_number, from_number, message, response);
    
    if (success) {
      Serial.println("Sent Message Successfully");
      ThingSpeak.setField(1,String(lati));
      ThingSpeak.setField(2,String(loni));
      ThingSpeak.writeFields(TS_Channel_ID,TS_API);
      display.clear();
      display.setFont(ArialMT_Plain_10);
      display.drawString(0,24,"Sent "); 
      display.drawString(25,24,"message ");
      display.drawString(70,24,"Sucessfully! ");
      display.display();
      delay(2000);
    } else {
      Serial.println("Message Not Sent");
      display.clear();
      display.setFont(ArialMT_Plain_10);
      display.drawString(0,24,response);
      display.display();
      delay(2000);
   }
    
  } else {
    Serial.print("No Data found for gps");
    display.clear();
    display.setFont(ArialMT_Plain_10);
    display.drawString(0,24,"No Data from GPS");
    display.display();
    delay(2000);
  }
} 
