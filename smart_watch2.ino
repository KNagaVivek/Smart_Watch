//Below 3 are related blynk 
#define BLYNK_TEMPLATE_ID "TMPLB1HnvOp2"
#define BLYNK_DEVICE_NAME "Smart Watch GPS"
#define BLYNK_AUTH_TOKEN "V3uWIiY_L8B-fryeKyLvi56IZWysIIaw"


#include <driver/rtc_io.h>
#include <WiFi.h>
#include "time.h"
#include "SSD1306.h"
#include <Wire.h>
#include <TinyGPS++.h>
#include "twilio.hpp"
#include "ThingSpeak.h";
#include <WiFiClient.h>
#include <BlynkSimpleEsp32.h>



#define BUTTON_PIN 19            //defining the push button pin
//Next 2 lines are id & api key for Thingspeak
#define TS_Channel_ID 1679754
#define TS_API "ZCAZXFTOSJJO6MU9"


SSD1306 display(0x3c, 21, 22);  //definig the oled display with its address,SCL & SDA connected pins
int c = 0;
int ls = LOW;  // the previous state from the input pin
int cs;     // the current reading from the input pin

WiFiClient client;

char auth[] = BLYNK_AUTH_TOKEN;  

static const char *ssid = "V2037";
static const char *password = "lokesh@123";


//Below are related to Twilio
//static const char *account_sid = "AC6296e127859b6735be7bb5a45ad2a5a6";
//static const char *account_sid = "AC1a3e0e5fdb382fc7adb6444a2b370bd8";
static const char *account_sid = "ACf96ce7f1be0906af105b6f9e85005293";
//static const char *auth_token = "66de627e23592b3d0d92751fb313f710";
//static const char *auth_token = "a3be44ebba590dfc1afbc26c709b1f67";
static const char *auth_token = "0dff126c7e715b049ef9565ecd0cba66";
// Phone number should start with "+<countrycode>"
//static const char *from_number = "+19107668616";
//static const char *from_number = "+16603337536";
static const char *from_number = "+14709446734";


// You choose!
// Phone number should start with "+1910"
static const char *to_number = "+919160401792";
String message = "Sent from ESP32:\n";


//For displaying the time we use
const char* ntpServer = "pool.ntp.org";
const long gmtOffset_sec = 19800;
const int  daylightOffset_sec = 0;

char lati[10],loni[10];

#define RXD2 16
#define TXD2 17
HardwareSerial neogps(1);

TinyGPSPlus gps;

Twilio *twilio;

BlynkTimer timer;

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

void senddata() {
  if(checkGPS()) {
    Blynk.virtualWrite(V0, lati);
    Blynk.virtualWrite(V1, loni);
    Serial.println(lati);
    Serial.println(loni);
  }
}

boolean checkGPS() {
  for (unsigned long start = millis(); millis() - start < 1000;){
      while (neogps.available()){
        if (gps.encode(neogps.read())) {
          return true;
        }
      }
    }
    return false;
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
  Blynk.begin(auth,ssid, password);
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
    boolean newData = checkGPS();
    
  if(newData == true) {
    newData = false;
    timer.setInterval(1000L, senddata);
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
  Blynk.run();
  timer.run();
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

    dtostrf(gps.location.lat(),2,6,lati);
    dtostrf(gps.location.lng(),2,6,loni);
    
    message += "I am in trouble pls help me. I am in this location: https://www.google.com/maps/?q=";
    message +=String(lati)+","+String(loni)+"\n";
    String response;
    bool success = twilio->send_message(to_number, from_number, message, response);
    
    if (success) {
      Serial.println("Sent Message Successfully");
      display.clear();
      display.setFont(ArialMT_Plain_10);
      display.drawString(0,24,"Sent "); 
      display.drawString(25,24,"message ");
      display.drawString(70,24,"Sucessfully! ");
      display.display();
      delay(2000);
      ThingSpeak.setField(1,String(lati));
      ThingSpeak.setField(2,String(loni));
      ThingSpeak.writeFields(TS_Channel_ID,TS_API);
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
