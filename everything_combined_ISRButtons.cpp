#include <Arduino.h>
#include <WiFi.h>
#include <Wire.h>
#include <AsyncTCP.h>
#include <ESPAsyncWebServer.h>
#include <Arduino_JSON.h>
#include <ESP32Servo.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

#define drop 27
#define SCREEN_WIDTH 128 // OLED display width, in pixels
#define SCREEN_HEIGHT 64 // OLED display height, in pixels
#define PBUTTON_PIN 33
#define MBUTTON_PIN 32
#define servo_pin 17

// Declaration for an SSD1306 display connected to I2C (SDA, SCL pins)
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, -1);

int adcval = 0;
long time1 = 0;
int drip;
const int threshold = 600;
int mph;

int counter1 = 0;


hw_timer_t *My_timer = NULL;

// Replace with your network credentials
const char* ssid = "Keerthi";
const char* password = "keerthi123";

Servo myservo;


const char index_html[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html>
  <head>
    <title>ESP IOT DASHBOARD</title>
    <meta name="viewport" content="width=device-width, initial-scale=1">
    <link rel="icon" type="image/png" href="favicon.png">
    <link rel="stylesheet" href="https://use.fontawesome.com/releases/v5.7.2/css/all.css" integrity="sha384-fnmOCqbTlWIlj8LyTjo7mOUStjsKC4pOpQbqyi7RrhN7udi9RwhKkMHpvLbHG9Sr" crossorigin="anonymous">
    <style>
      html {
      font-family: Arial, Helvetica, sans-serif; 
      display: inline-block; 
      text-align: center;
      }
      h1 {
        font-size: 1.8rem; 
        color: white;
      }
      p { 
        font-size: 1.4rem;
      }
      .topnav { 
        overflow: hidden; 
        background-color: #0A1128;
      }
      body {  
        margin: 0;
      }
      .content { 
        padding: 5%;
      }
      .card-grid { 
        max-width: 1200px; 
        margin: 0 auto; 
        display: grid; 
        grid-gap: 2rem; 
        grid-template-columns: repeat(auto-fit, minmax(200px, 1fr));
      }
      .card { 
        background-color: white; 
        box-shadow: 2px 2px 12px 1px rgba(140,140,140,.5);
      }
      .card-title { 
        font-size: 1.2rem;
        font-weight: bold;
        color: #034078
      }
      .slider { 
        -webkit-appearance: none; 
        margin: 14px; 
        width: 60%; 
        height: 25px; 
        background: #FFD65C;
        outline: none; 
        -webkit-transition: .2s; 
        transition: opacity .2s;
      }
      .slider::-webkit-slider-thumb {
        -webkit-appearance: none;
        appearance: none; 
        width: 5%; 
        height: 35px; 
        background: #003249; 
        cursor: pointer;
      }
      .slider::-moz-range-thumb { 
        width: 5%; 
        height: 35px; 
        background: #003249; 
        cursor: pointer; 
      }
    </style>
    <!-- <script src="http://cdn.rawgit.com/Mikhus/canvas-gauges/gh-pages/download/2.1.7/all/gauge.min.js"></script> -->
  </head>
  <body>
    <div class="topnav">
      <h1>ACM SMART DRIP SYSTEMS - IoT</h1>
    </div>
    <div class="content">
      <div class="card-grid">
        <div class="card">
          <p class="card-title">Drip Rate:<span id="gauge-dripRate"></span></p>
        </div>
    </div>
    <div class="content">
        <div class="card"><br>
            <p class="card-title">Control The drip Rate Here:</p>
            <p><span id="textSliderValue">Welcome to ACM NITK!</span></p>
            <p><input type="range" onchange="updateSliderPWM(this)" id="pwmSlider" min="85" max="119" value="%SLIDERVALUE%" step="1" class="slider"></p>    
        </div>
      </div>
    </div>
    <script>
    // Get current sensor readings when the page loads  
      window.addEventListener('load', getReadings);

      var gaugeHum = document.getElementById("gauge-dripRate")
      // Function to get current readings on the webpage when it loads for the first time
      function getReadings(){
        var xhr = new XMLHttpRequest();
        xhr.onreadystatechange = function() {
          if (this.readyState == 4 && this.status == 200) {
            var myObj = JSON.parse(this.responseText);
            console.log(myObj);
            var hum = myObj.dripRate;
            gaugeHum.innerHTML = hum;
          }
        }; 
        xhr.open("GET", "/readings", true);
        xhr.send();
      }

      if (!!window.EventSource) {
        var source = new EventSource('/events');
        
        source.addEventListener('open', function(e) {
          console.log("Events Connected");
        }, false);

        source.addEventListener('error', function(e) {
          if (e.target.readyState != EventSource.OPEN) {
            console.log("Events Disconnected");
          }
        }, false);
        
        source.addEventListener('message', function(e) {
          console.log("message", e.data);
        }, false);
        
        source.addEventListener('new_readings', function(e) {
          console.log("new_readings", e.data);
          var myObj = JSON.parse(e.data);
          console.log(myObj);
          gaugeHum.innerHTML = myObj.dripRate;
        }, false);
      }
      function updateSliderPWM(element) {
        var sliderValue = document.getElementById("pwmSlider").value;
        document.getElementById("textSliderValue").innerHTML = sliderValue;
        console.log(sliderValue);
        var xhr = new XMLHttpRequest();
        xhr.open("GET", "/slider?value="+sliderValue, true);
        xhr.send();
      }
      </script>
  </body>
</html>
)rawliteral";

const char* PARAM_INPUT = "value";
String sliderValue = "0";
int prevSliderValue = 0;
int servoAngle = 0;

// Create AsyncWebServer object on port 80
AsyncWebServer server(80);

// Create an Event Source on /events
AsyncEventSource events("/events");

// Json Variable to Hold Sensor Readings
JSONVar readings;

// Timer variables
unsigned long lastTime = 0;
unsigned long timerDelay = 1000;

void increase_servo()
{
    servoAngle = servoAngle + 1;
    myservo.write(servoAngle);
}

void decrease_servo()
{
    servoAngle = servoAngle - 1;
    myservo.write(servoAngle);
}

void IRAM_ATTR onTimer(){
 adcval = digitalRead(drop);
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

// Get Sensor Readings and return JSON object
String getSensorReadings(){
  readings["dripRate"] = String(mph);
  String jsonString = JSON.stringify(readings);
  return jsonString;
}

// // Initialize SPIFFS
// void initSPIFFS() {
//   if (!SPIFFS.begin()) {
//     Serial.println("An error has occurred while mounting SPIFFS");
//   }
//   Serial.println("SPIFFS mounted successfully");
// }

// Initialize WiFi
void initWiFi() {
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);
  Serial.print("Connecting to WiFi ..");
  while (WiFi.status() != WL_CONNECTED) {
    Serial.print('.');
    delay(1000);
  }
  Serial.println(WiFi.localIP());
}

String processor(const String& var){
  //Serial.println(var);
  if (var == "SLIDERVALUE"){
    return sliderValue;
  }
  return String();
}

void setup() {
  // Serial port for debugging purposes
  Serial.begin(115200);
  initWiFi();
  pinMode(PBUTTON_PIN, INPUT_PULLUP);
  pinMode(MBUTTON_PIN, INPUT_PULLUP);
  attachInterrupt(digitalPinToInterrupt(PBUTTON_PIN), increase_servo, FALLING);
  attachInterrupt(digitalPinToInterrupt(MBUTTON_PIN), decrease_servo, FALLING);

  myservo.attach(servo_pin);

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

  // Web Server Root URL
  server.on("/", HTTP_GET, [](AsyncWebServerRequest *request){
    request->send(200, "text/html", index_html);
  });

  // Request for the latest sensor readings
  server.on("/readings", HTTP_GET, [](AsyncWebServerRequest *request){
    String json = getSensorReadings();
    request->send(200, "application/json", json);
    json = String();
  });

  // Send a GET request to <ESP_IP>/slider?value=<inputMessage>
  server.on("/slider", HTTP_GET, [] (AsyncWebServerRequest *request) {
    String inputMessage;
    // GET input1 value on <ESP_IP>/slider?value=<inputMessage>
    if (request->hasParam(PARAM_INPUT)) {
      inputMessage = request->getParam(PARAM_INPUT)->value();
      prevSliderValue = sliderValue.toInt();
      sliderValue = inputMessage;
      servoAngle = sliderValue.toInt();
      // Write Servo.write here
      myservo.write(servoAngle);
      Serial.println("Slider Value = "+sliderValue);
    }
    else {
      inputMessage = "No message sent";
    }
    Serial.println(inputMessage);
    request->send(200, "text/plain", "OK");
  });

  events.onConnect([](AsyncEventSourceClient *client){
    if(client->lastId()){
      Serial.printf("Client reconnected! Last message ID that it got is: %u\n", client->lastId());
    }
    // send event with message "hello!", id current millis
    // and set reconnect delay to 1 second
    client->send("hello!", NULL, millis(), 10000);
  });
  server.addHandler(&events);

  // Start server
  server.begin();
}

void loop() {
  if(mph!=0){
    mph = 60/mph;
  }
  if ((millis() - lastTime) > timerDelay) {
    // Send Events to the client with the Sensor Readings Every 10 seconds
    events.send("ping",NULL,millis());
    events.send(getSensorReadings().c_str(),"new_readings" ,millis());
    lastTime = millis();
    
    display.clearDisplay();

    // display temperature
    display.setTextSize(2);
    display.setCursor(0,0);
    display.print("Drops per minute: ");
    display.setTextSize(2);
    display.setCursor(0,35);
    display.print(mph);
    display.display();
  }

//   if(prevSliderValue != sliderValue){
//     myservo.write(sliderValue);
//   }
//   else{
    // if(digitalRead(PBUTTON_PIN) == LOW){
    //     servoAngle = servoAngle + 1;
    //     myservo.write(servoAngle);
    // }
    // else if(digitalRead(MBUTTON_PIN) == LOW){
    //     servoAngle = servoAngle - 1;
    //     myservo.write(servoAngle);
    // }
    // myservo.write(servoAngle);
    // Serial.println("Servo Angle = "+servoAngle);
}
