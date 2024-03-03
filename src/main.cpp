#include <Arduino.h>
#include <ESP8266WiFi.h>
#include <ESPAsyncWebServer.h>
#include <LittleFS.h>
#include <helperfunctions.h>

// Wi-Fi and webserver settings
const char* ssid = "ssid";
const char* password = "password";
AsyncWebServer server(80);

// Define the relay trigger based on the board config
const uint8_t relayOn = HIGH;
const uint8_t relayOff = LOW;

// Set  GPIO
const int relayPin = 5; // relay connected to  GPIO5
const int ledPin = 2;

// Store the watering state
String wateringState;

// Stores the interval for which watering was set
String interval = "";

// time variables 
long prevReadingTime = 0;
long wateringInterval = 0;
long postpone_seconds = 0;
long remainingWateringTime = 0;


// Replace in the mgmt webpage the placeholder with actual watering state
String processor(const String& var){
  if(var == "STATE"){
    if(digitalRead(relayPin) == relayOn){
      wateringState = "ON";
    }
    else{
      wateringState = "OFF";
    }
    return wateringState;
  }
  else {
    return "Nothing";
  }
};


// callback function to serve index
void start(AsyncWebServerRequest *request)
{
  //web request is as /start?interval=<intervalParameter>
  // check how many parameters were received
  int paramsNr = request->params(); 
  for(int i=0;i<paramsNr;i++){
      AsyncWebParameter* p = request->getParam(i);
      if (p->name() == "interval"){
        interval = p->value();
        break;
      }
      else{
        // if needed use for further logic to handle calls without interval value
      }
  }

  // Determine the requested watering interval and start resp. water relay
  switch (resolveParameters(interval))
  {
    case OneHour:
      wateringInterval = 1*60*60*1000; // in milliseconds = 1 hour * 60 min * 60 sec * 1000
      break;
    case TwoHours:
      wateringInterval = 2*60*60*1000; // in milliseconds = 2 hours * 60 min * 60 sec * 1000
      break;
    case ThreeHours:
      wateringInterval = 3*60*60*1000; // in milliseconds = 3 hours * 60 min * 60 sec * 1000
      break;
    default:
      break;
  }

  // turn on the relay
  digitalWrite(relayPin, relayOn);

  //turn on the LED to indicate relay is engaged
  digitalWrite(ledPin, LOW);    

  prevReadingTime = millis();
  remainingWateringTime =  wateringInterval;

  request->send(LittleFS, "/index.html", String(), false, processor); 
}


void setup() {
  Serial.begin(74880);

  // Initialize SPIFFS 
  if(!LittleFS.begin()){
    Serial.println("An Error has occurred while mounting LittleFS");
    return;
  }

  // Configure the relay pin as output and set it to Low
  digitalWrite(relayPin, relayOff);
  pinMode(relayPin,OUTPUT);

  //make sure LED is OFF as relay is not engaged
  pinMode(ledPin,OUTPUT);
  digitalWrite(ledPin, HIGH);

  // Connect to Wi-Fi
  WiFi.hostname("micro-drip-relay");
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);
  Serial.println("");

  // Wait for connection
  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    Serial.println("Connecting to WiFi...");
  }

  // Print Wi-Fi connection info
  Serial.println("");
  Serial.print("Connected to ");
  Serial.println(ssid);
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());

  // Route for root / web page
  server.on("/", HTTP_GET, [](AsyncWebServerRequest *request){
    request->send(LittleFS, "/index.html", String(), false, processor);
  });
  
  // Route to load style.css file
  server.on("/style.css", HTTP_GET, [](AsyncWebServerRequest *request){
    request->send(LittleFS, "/style.css", "text/css");
  });

  // Route to start the watering with a given internval
  server.on("/start", HTTP_GET, start);  

  // Route to turn off the watering
  server.on("/off", HTTP_GET, [](AsyncWebServerRequest *request){
    // reset interval
    remainingWateringTime = 0;
    prevReadingTime = 0;
    String lastSetInterval = interval; 
    interval = ""; 
    // return the last set interval so UI button gets deactivated
    request->send(200, "text/plain", lastSetInterval.c_str()); 
  });

  // Route to serve the remaining watering time
  server.on("/remainingWateringTime", HTTP_GET, [](AsyncWebServerRequest *request){
    if (remainingWateringTime <= 0)
    {
      Serial.println("watering inactive");
    }
    else
    {
      // Serial.println("watering ongoing... ");
    }
    String retval = "{\"remainingSeconds\": " + String(remainingWateringTime/1000) + " ,\"intervalSet\": \"" + interval + "\"}";
    // add to the return value information which watering interval was set
    request->send(200, "text/plain", retval.c_str()); 
  });

  // Start server
  server.begin();

}

void loop() {
  // update the remaining time to keep water on
  if (remainingWateringTime > 0)
  {
    long now = millis();
    remainingWateringTime = remainingWateringTime - (now - prevReadingTime);
    prevReadingTime = now;
  }
  else if (remainingWateringTime == 0)
  {
    // turn off the relay  
    digitalWrite(relayPin, relayOff);  

    //turn off the LED to indicate relay is now disengaged
    digitalWrite(ledPin, HIGH);

    // reset the intervals
    remainingWateringTime = -1;
    prevReadingTime = 0;
    interval = "";
  }
}

