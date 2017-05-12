/* This code was built from referencing a number of sources including examples
 * from Blynk, ArduinoOTA, ESP8266Wifi, and Nicoo's Web Controlled Water System
 * You should be able to follow which pins are connected where
 * I have the Blynk console with a push button switch on V2, a holding switch
 * on V3, and LEDs tied to V4, V5, and V6 to light up on the phone when a zone
 * is running.  I used the Eventor widget to trigger V2 on a timer at specific
 * times on the clock.  I also have a terminal widget tied to V1.
 * The purpose of this project is to open solenoid valves hooked up after an
 * RV pump (that turns on automatically when pressure on the back side drops
 * like when a valve opens up) that pumps water from my rain barrels.  But I
 * do not want the pump to run when there is no water in the barrels, so I have
 * an ultrasonic distance measurer to ping the surface of the water to make sure
 * it is OK to open the valve.  The water level is checked at each zone.  Feedback
 * is posted to the terminal widget so I can review what happened later. 
 * I have two different relay boards; one uses LOW to flip the relay, the other
 * uses HIGH to flip the relay.  So I used a #define at the beginning of the 
 * project to switch it when I change boards.  

 Frank Voss
 May 1, 2017  */

#include "Timer.h"
#include <BlynkSimpleEsp8266.h>
#include <ESP8266WiFi.h>
#include <ESP8266mDNS.h> // Needed for OTA
#include <WiFiUdp.h>     // Needed for OTA
#include <ArduinoOTA.h>  // Needed for OTA

Timer t;
#define NUM_SWITCH 3
#define MAX_TIME .25*60 //in seconds
#define OFF 1
#define ON 0

// You should get Auth Token in the Blynk App.
// Go to the Project Settings (nut icon).
char auth[] = "Auth Token";

// Your WiFi credentials.
// Set password to "" for open networks.
const char* ssid = "SSID
const char* password = "PASSWORD";
IPAddress ip(XX, XX, XX, XX); // where xx is the desired IP Address
IPAddress gateway(XX, XX, XX, XX); // set gateway to match your network
IPAddress subnet(XX, XX, XX, XX); // set subnet mask to match your network
//const char* ssid = "SSID2";
//const char* password = "PASSWORD2";
//IPAddress ip(XX, XX, XX, XX); // where xx is the desired IP Address
//IPAddress gateway(XX, XX, XX, XX); // set gateway to match your network
//IPAddress subnet(XX, XX, XX, XX); // set subnet mask to match your network



//int relaypin[NUM_SWITCH] = {2,3,4};
//int relaytime[NUM_SWITCH] = {.25*60, .5*60, .1*60};
//byte on_off[NUM_SWITCH];
int digitalInPin = 12;
int V3State = 1;
int V2State = 0;
//ultrasound setup
int trigPin = D10;    //Trig
int echoPin = D11;    //Echo
double duration, distance;

// Attach virtual serial terminal to Virtual Pin V1
WidgetTerminal terminal(V1);

// You can send commands from Terminal to your hardware. Just use
// the same Virtual Pin as your Terminal Widget
BLYNK_WRITE(V1)
{
  // if you type "Marco" into Terminal Widget - it will respond: "Polo:"
  if (String("Marco") == param.asStr()) {
    terminal.println("You said: 'Marco'") ;
    terminal.println("I said: 'Polo'") ;
  // if you type "distance" into Terminal Widget  - it will check the distance to the water
  } else if (String("dist") == param.asStr()){
    distance = Checkdistance();
    terminalclear();
    terminal.print("Distance to water is ");
    terminal.print(distance);
    terminal.println(" inches");
    terminal.flush();
  // if you type "clear" into Terminal Widget - it will clear the terminal screen
  } else if (String("clear") == param.asStr()){
    terminalclear();
  } else {  
    // if you type anything else, it will send it back to you
    terminal.print("You said:");
    terminal.write(param.getBuffer(), param.getLength());
    terminal.println();
  }

  // Ensure everything is sent
  terminal.flush();
}

void terminalclear(){
  for (int x=0; x<7; x++){
  terminal.println();
  }
}

BLYNK_WRITE(V2){
  if (param.asInt()) {
        //HIGH
//        pin_activated(D2, V2); // This runs a function to activate D2 based on the input from V2
     V2State = HIGH;
    } else {
       //LOW
//       pin_deactivated(D2, V2);  }  // This runs a function to deactivate D2 based on the input from V2
     V2State = LOW;
    }
}

BLYNK_WRITE(V3){
  if (param.asInt()) {
       //HIGH
       //pin_activated(D4, V3);
       V3State = HIGH;
    } else {
       //LOW
       //pin_deactivated(D4, V3);  
       V3State = LOW;}
}

//int D2=2;  //comment out these lines
//int D3=3;  //for use with WEMOS with Blynk
//int D4=4;  //because the "D" and "V" 
//int D10=10;
//int D11=11;
//int D12=12;
//int V4=4;  //pin designations are needed
//int V5=5;  //but do not work with 
//int V6=6;  //Arduino/Genuino

int zonearray[NUM_SWITCH][6] = {
  {1,D2,V4,15,OFF,0},  //zone number, zone pin, virtual pin, time on, pin on_off, vpin on_off (1=off, 0=on)
  {2,D3,V5,18,OFF,0},
  {3,D4,V6,10,OFF,0}
};

//timer
int seconds = 0;



void callback()
{
  Serial.print("seconds = ");
  Serial.println(seconds);

  if (seconds == 1)
    {
    //next zone
    Serial.println("Next zone");
//    seconds = MAX_TIME;
    int swi, was_on = -1;
    for (swi=0; swi<NUM_SWITCH; swi++)
    {
      //if (on_off[swi]==HIGH)
      if (zonearray[swi][4] == ON)
        was_on = swi;
      //seconds = relaytime[was_on+1];
      //on_off[swi]=LOW;
      seconds = zonearray[was_on+1][3];
      zonearray[swi][4] = OFF;
      zonearray[swi][5] = 0;
     }
     Serial.print("was_on = ");
     Serial.println(was_on);
     if (was_on+1 < NUM_SWITCH)
     {
       double distance = Checkdistance();
       if (distance < 48)
       {
         //on_off[was_on+1]=HIGH;
         //seconds = relaytime[was_on+1];
         seconds = zonearray[was_on+1][3];
         zonearray[was_on+1][4] = ON;
         zonearray[was_on+1][5] = 255;
         terminal.print("Running Zone ");
         terminal.print(was_on+2);
         terminal.println(".");
         terminal.flush();
       }
       else
         seconds = 0;
     }
     for (swi=0; swi<NUM_SWITCH; swi++){
       //digitalWrite(relaypin[swi], on_off[swi]);
       digitalWrite(zonearray[swi][1], zonearray[swi][4]);
       Blynk.virtualWrite(zonearray[swi][2], zonearray[swi][5]);
     }
     delay(500);  //cheap debounce
     if (was_on + 1 == NUM_SWITCH)
     {
      terminal.println("Turning everything off");
      terminal.flush();
      int swi;
      for (swi=0; swi<NUM_SWITCH; swi++)
        {
        //on_off[swi] = LOW;
        //digitalWrite(relaypin[swi], on_off[swi]);
        zonearray[swi][4] = OFF;
        zonearray[swi][5] = 0;
        digitalWrite(zonearray[swi][1], zonearray[swi][4]);
        Blynk.virtualWrite(zonearray[swi][2], zonearray[swi][5]);
        }
      seconds = 0;
      }
    }
  if (seconds)
    seconds--;
}
//end of timer

double Checkdistance(){
  double inches;
  // The sensor is triggered by a HIGH pulse of 10 or more microseconds.
  // Give a short LOW pulse beforehand to ensure a clean HIGH pulse:  
  digitalWrite(trigPin, LOW);
  delayMicroseconds(50);
  digitalWrite(trigPin, HIGH);
  delayMicroseconds(20); // Added this line
  digitalWrite(trigPin, LOW);
  duration = pulseIn(echoPin, HIGH);
  inches = (duration/2)/74;
  terminal.print("Distance to water is ");
  terminal.print(inches);
  terminal.println(" inches");
  terminal.flush();
  return inches;
}

void turnalloff(){
      terminal.println(F("Turning everything off"));
      int swi;
      for (swi=0; swi<NUM_SWITCH; swi++)
        {
        //on_off[swi] = LOW;
        //digitalWrite(relaypin[swi], on_off[swi]);
        zonearray[swi][4] = OFF;
        zonearray[swi][5] = 0;
        digitalWrite(zonearray[swi][1], zonearray[swi][4]);
        Blynk.virtualWrite(zonearray[swi][2], zonearray[swi][5]);
        }
      seconds = 0;
}

int updateVpinstates(int vpin){
  switch ( vpin){
    case V2:
      V2State = LOW;
      break;
    case V3:
      V3State = LOW;
      break;
  }
}

void setup()
{
    // Debug console
  Serial.begin(115200);

  WiFi.config(ip, gateway, subnet); 
  WiFi.begin(ssid, password);
  
  Blynk.config(auth);
  Blynk.begin(auth,ssid,password);
  // This will print Blynk Software version to the Terminal Widget when
  // your hardware gets connected to Blynk Server
  terminal.println(F("Blynk v" BLYNK_VERSION ": Device started"));
  terminal.println(F("-------------"));
  terminal.println(F("Type 'dist' to check water level or"));
  terminal.println(F("anything else and get it printed back."));
  terminal.flush();
  //end blynk start info

//This info is needed for OTA
  // Port defaults to 8266
  // ArduinoOTA.setPort(8266);

  // Hostname defaults to esp8266-[ChipID]
   ArduinoOTA.setHostname("Wemos");

  // No authentication by default
   ArduinoOTA.setPassword("admin");

  // Password can be set with it's md5 value as well
  // MD5(admin) = 21232f297a57a5a743894a0e4a801fc3
  // ArduinoOTA.setPasswordHash("21232f297a57a5a743894a0e4a801fc3");

  ArduinoOTA.onStart([]() {
    String type;
    if (ArduinoOTA.getCommand() == U_FLASH)
      type = "sketch";
    else // U_SPIFFS
      type = "filesystem";

    // NOTE: if updating SPIFFS this would be the place to unmount SPIFFS using SPIFFS.end()
    Serial.println("Start updating " + type);
  });
  ArduinoOTA.onEnd([]() {
    Serial.println("\nEnd");
  });
  ArduinoOTA.onProgress([](unsigned int progress, unsigned int total) {
    Serial.printf("Progress: %u%%\r", (progress / (total / 100)));
  });
  ArduinoOTA.onError([](ota_error_t error) {
    Serial.printf("Error[%u]: ", error);
    if (error == OTA_AUTH_ERROR) Serial.println("Auth Failed");
    else if (error == OTA_BEGIN_ERROR) Serial.println("Begin Failed");
    else if (error == OTA_CONNECT_ERROR) Serial.println("Connect Failed");
    else if (error == OTA_RECEIVE_ERROR) Serial.println("Receive Failed");
    else if (error == OTA_END_ERROR) Serial.println("End Failed");
  });
  ArduinoOTA.begin();
  Serial.println("Ready");
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());
//End info for OTA  
  
  //default state is off
  int i;
  for(i=0; i<NUM_SWITCH; i++)
    //on_off[i]= LOW;
    zonearray[i][4]=OFF;
    zonearray[i][5] = 0;

  //setup seconds timer
  t.every(1000, callback);

  //manual control;
  pinMode(digitalInPin, INPUT);

  int swi;
  for (swi=0; swi<NUM_SWITCH; swi++)
  {
    //pinMode(relaypin[swi], OUTPUT);
    //digitalWrite(relaypin[swi], on_off[swi]);
    pinMode(zonearray[swi][1], OUTPUT);
    digitalWrite(zonearray[swi][1], zonearray[swi][4]);
    Blynk.virtualWrite(zonearray[swi][2], zonearray[swi][5]);
  }
  
  //ultrasound setup
  pinMode(trigPin, OUTPUT);
  pinMode(echoPin, INPUT);
}

void loop()
{
  ArduinoOTA.handle(); //Eliminate this line if no OTA
  Blynk.run();
  
  int val = digitalRead(digitalInPin);
  if (((val)||(V2State)) && (V3State)) //button was pressed and cancel is off
  {
    Blynk.virtualWrite(V2,0);
    updateVpinstates(V2);
    //seconds = relaytime[0];
    seconds = zonearray[0][3];
    //cycle through circuits
    int swi, was_on = -1;
    for (swi=0; swi<NUM_SWITCH; swi++)
    {
      //if (on_off[swi]==HIGH)
      if (zonearray[swi][4]==ON)  
        was_on = swi;
      //on_off[swi]=LOW;
      zonearray[swi][4]=OFF;
      zonearray[swi][5] = 0;
     }
     if (was_on+1 < NUM_SWITCH)
     {
       distance = Checkdistance();
       if (distance < 48)
       {
         //on_off[was_on+1]=HIGH;
         //seconds = relaytime[was_on+1];
         zonearray[was_on+1][4]=ON;
         zonearray[was_on+1][5] = 255;
         seconds = zonearray[was_on+1][3];
         terminal.print("Running Zone ");
         terminal.print(was_on+2);
         terminal.println(".");
         terminal.flush();
       }
       else
         seconds = 0;

     }
     if (was_on+1 == NUM_SWITCH)
       seconds = 0;
     for (swi=0; swi<NUM_SWITCH; swi++){
       //digitalWrite(relaypin[swi], on_off[swi]);
       digitalWrite(zonearray[swi][1], zonearray[swi][4]);
       Blynk.virtualWrite(zonearray[swi][2], zonearray[swi][5]);
     }
       delay(500); //cheap de-bounce

     
     return;
  }
  //end manual control
  if (!V3State) //V3 state is supposed to match the virtual blynk pin on the phone
    if (seconds != 0)
    turnalloff(); //It is set up now to be 1 when not "lit" and 0 when lit

    
  t.update();
}

