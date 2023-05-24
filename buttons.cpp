#include <WiFi.h>
#include <Wire.h>
#include "ThingSpeak.h"

#define PBUTTON_PIN 33
#define MBUTTON_PIN 32

unsigned int last_interrupt_time = 0; 
unsigned int interrupt_time = 0; 
unsigned long int angle = 0;
unsigned int statusCode;

const char* ssid = "Keerthi";   // your network SSID (name) 
const char* password = "keerthi123";   // your network password

WiFiClient  client;

unsigned int myChannelNumber = 2081441;
const char * myWriteAPIKey = "T9P3ZVNLLWZM0QWU";
unsigned int counterChannelNumber = 2081441;            // Channel ID
const char * myCounterReadAPIKey = "B9KYKQ7I939PPNSF";    // Read API Key

void increase_servo()
{
 interrupt_time = millis();

 if (interrupt_time - last_interrupt_time > 200) 
 {
   angle++;
   last_interrupt_time = interrupt_time;
 }


}
void decrease_servo()
{
 interrupt_time = millis();

 if (interrupt_time - last_interrupt_time > 200) 
 {
   angle--;
   last_interrupt_time = interrupt_time;
 }

}
void setup() {
  Serial.begin(115200);
  pinMode(PBUTTON_PIN, INPUT_PULLUP);
  pinMode(MBUTTON_PIN, INPUT_PULLUP);
  attachInterrupt(digitalPinToInterrupt(PBUTTON_PIN), increase_servo, FALLING);
  attachInterrupt(digitalPinToInterrupt(MBUTTON_PIN), decrease_servo, FALLING);
  WiFi.mode(WIFI_STA);
  ThingSpeak.begin(client);
}

void loop() {
   // Connect or reconnect to WiFi
    if(WiFi.status() != WL_CONNECTED){
      Serial.print("Attempting to connect");
      while(WiFi.status() != WL_CONNECTED){
        WiFi.begin(ssid, password); 
        delay(5000);     
      } 
      Serial.println("\nConnected.");
    }

    
  // statusCode = ThingSpeak.writeField(myChannelNumber, 2, angle, myWriteAPIKey);
  // if(statusCode == 200){
  //   Serial.println("Channel update successful.");
  // }
  // else{
  //   Serial.println("Problem updating channel. HTTP error code " + String(statusCode));
  // }
    

  angle = ThingSpeak.readLongField(counterChannelNumber, 2, myCounterReadAPIKey);      // FieldNumber = 2
  statusCode = ThingSpeak.getLastReadStatus();
  if (statusCode == 200)
  {
    Serial.print("Angle: ");
    // Myservo.write(angle);
    Serial.println(angle);
  }
  else
  {
    Serial.println("Unable to read channel / No internet connection");
  }
  delay(100);
  
  Serial.println(angle);
  delay(100);
}