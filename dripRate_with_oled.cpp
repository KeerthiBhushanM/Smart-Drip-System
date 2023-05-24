#include <WiFi.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include "ThingSpeak.h"

#define drop 27
#define SCREEN_WIDTH 128 // OLED display width, in pixels
#define SCREEN_HEIGHT 64 // OLED display height, in pixels

// Declaration for an SSD1306 display connected to I2C (SDA, SCL pins)
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, -1);

int adcval = 0;
long time1 = 0;
int drip;
const int threshold = 600;
int mph;

int counter1 = 0;


hw_timer_t *My_timer = NULL;

const char* ssid = "Keerthi";   // your network SSID (name) 
const char* password = "keerthi123";   // your network password

WiFiClient  client;

unsigned long myChannelNumber = 1;
const char * myWriteAPIKey = "T9P3ZVNLLWZM0QWU";

// Timer variables
unsigned long lastTime = 0;
unsigned long timerDelay = 10000;

// Variable to hold temperature readings

//uncomment if you want to get temperature in Fahrenheit
//float temperatureF;

// Create a sensor object
 //BME280 connect to ESP32 I2C (GPIO 21 = SDA, GPIO 22 = SCL)

void IRAM_ATTR onTimer(){
  // Serial.print("called");
 adcval = digitalRead(drop);
//  Serial.print(adcval); // 
//  Serial.println(mph); // 
    if (adcval == LOW)
    { // if reed switch is closed
            mph = time1;   
    //Serial.println(mph);
            
            time1 = 0;                    // reset timer
    }
    if (time1 > 1500)
    {
        mph = 0;
         // if no new pulses from reed switch- tire is still, set mph to 0
    }
    else
    {
        time1 += 1; // increment timer
    }
}


void setup() {
  Serial.begin(115200);  //Initialize serial
  
  
  WiFi.mode(WIFI_STA);   
  
  ThingSpeak.begin(client);  // Initialize ThingSpeak
  //pinMode(LED, OUTPUT);

  if(!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) {
    Serial.println(F("SSD1306 allocation failed"));
    for(;;);
  }
  delay(2000);
  display.clearDisplay();
  display.setTextColor(WHITE);

  My_timer = timerBegin(0, 80, true);
  timerAttachInterrupt(My_timer, &onTimer, true);
  timerAlarmWrite(My_timer, 100000, true);
  timerAlarmEnable(My_timer); //Just Enable
}

void loop() {
  // if ((millis() - lastTime) > timerDelay) {
    
    // Connect or reconnect to WiFi
    if(WiFi.status() != WL_CONNECTED){
      Serial.print("Attempting to connect");
      while(WiFi.status() != WL_CONNECTED){
        WiFi.begin(ssid, password); 
        delay(5000);     
      } 
      Serial.println("\nConnected.");
    }

    // Get a new temperature reading
    // temperatureC = bme.readTemperature();
    // Serial.print("Temperature (ºC): ");
    // Serial.println(temperatureC);
    
    //uncomment if you want to get temperature in Fahrenheit
    /*temperatureF = 1.8 * bme.readTemperature() + 32;
    Serial.print("Temperature (ºC): ");
    Serial.println(temperatureF);*/
  if(mph!=0){
    mph = 60/mph;
  }
    
    // Serial.println(mph);
    // Write to ThingSpeak. There are up to 8 fields in a channel, allowing you to store up to 8 different
    // pieces of information in a channel.  Here, we write to field 1.
    // if (mph>1){
    //   int x = ThingSpeak.writeField(myChannelNumber, 1, mph, myWriteAPIKey);
    // }
    display.clearDisplay();
  
  // display temperature
    display.setTextSize(2);
    display.setCursor(0,0);
    display.print("Drops per minute: ");
    display.setTextSize(2);
    display.setCursor(0,35);
    display.print(mph);
    display.display();

    int x = ThingSpeak.writeField(myChannelNumber, 1, mph, myWriteAPIKey);
    //uncomment if you want to get temperature in Fahrenheit
    //int x = ThingSpeak.writeField(myChannelNumber, 1, temperatureF, myWriteAPIKey);

    if(x == 200){
      Serial.println("Channel update successful.");
    }
    else{
      Serial.println("Problem updating channel. HTTP error code " + String(x));
    }
    
  
}