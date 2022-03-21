#include <SPI.h>
#include <WiFiNINA.h>
#include <WiFiUdp.h>
#include <NTPClient.h>
#include <EEPROM.h>
#include <Wire.h>
#include "Grove_Human_Presence_Sensor.h"

#define sensorPin A5
#define led 5
#define buzzer 9
#define wifiLED 10

char ssid[] = "-SSID-";
char pass[] = "-Password-";
int keyIndex = 0;
unsigned long myTime;
boolean wifiStatus = false;

AK9753 movementSensor;
float sensitivity_presence = 2.0;
float sensitivity_movement = 3.0;
int detect_interval = 30; //milliseconds
PresenceDetector detector(movementSensor, sensitivity_presence, sensitivity_movement, detect_interval);
uint32_t last_time;

boolean connected = false;
char serveradd[] = "1.2.3.4"; //Server IP or domain name here
String postData;
String postVariable = "Data Uploaded (HH/MM/SS): ";
int place = -1;

int status = WL_IDLE_STATUS;
WiFiServer server(80);

//const long utcOffsetInSeconds = EEPROM.read(0)*60*60;
const long utcOffsetInSeconds = -5*60*60;
int addr = 0;
char daysOfTheWeek[7][12] = {"Sunday", "Monday", "Tuesday", "Wednesday", "Thursday", "Friday", "Saturday"};

WiFiUDP ntpUDP;
NTPClient timeClient(ntpUDP, "pool.ntp.org", utcOffsetInSeconds);

WiFiClient client;

String readString;

void setup() {
  pinMode(led, OUTPUT);
  Serial.begin(9600);

  while (status != WL_CONNECTED) {
    Serial.print("Attempting to connect to Network named: ");
    Serial.println(ssid);
    status = WiFi.begin(ssid, pass);
    delay(1000);
  }
  server.begin();
  digitalWrite(wifiLED, HIGH);
  wifiStatus = true;
  Serial.print("SSID: ");
  Serial.println(WiFi.SSID());
  IPAddress ip = WiFi.localIP();
  Serial.print("IP Address: ");
  Serial.println(ip);
  Wire.begin();

  if (movementSensor.initialize() == false) {
      Serial.println("Device not found. Check wiring.");
      while (1);
  }
  timeClient.begin();
}

void loop() 
{
  if (WiFi.status() != WL_CONNECTED) {
    if(wifiStatus == true)
    {
      wifiStatus = false;
      myTime = millis();
      Serial.println("Connection Lost");
      digitalWrite(wifiLED, LOW);
      Serial.print("Attempting to connect to Network named: ");
      Serial.println(ssid);
    }
    status = WiFi.begin(ssid, pass);
    //WiFiConnect();
  }
  else
  {
    if(wifiStatus == false)
    {
      wifiStatus = true;
      myTime = 0;
      Serial.println("Connected");
      digitalWrite(wifiLED, HIGH);
      webServer(-10);
    }      
  }
  sensor();
}

void webServer(long int val)
{
  postData = postVariable + val;

  if (client.connect(serveradd, 80)) {
    if (connected == false)
    {
      connected = true;
      Serial.print("Connected\n");
    }
    for (int j = place;j > -1; j-=3)
    {
      String a = "POST /test/post.php?temp=";
      uint32_t savedTime = 0;
      long int savedHours = EEPROM.read(place-2);
      int savedMinutes = EEPROM.read(place-1);
      int savedSeconds = EEPROM.read(place);
      savedTime = (10000*savedHours) + (100*savedMinutes) + (savedSeconds);
      //Serial.println(savedHours);
      //Serial.println(savedMinutes);
      //Serial.println(savedSeconds);
      savedTime = savedHours*10000 + 100*savedMinutes + savedSeconds;
      client.println(a + savedTime + " HTTP/1.1");
      client.println("Content-Type: application/x-www-form-urlencoded");
      client.print("Content-Length: ");
      client.println(postData.length());
      client.print("Host: ");
      client.println(serveradd);
      client.println();
      client.print(postData);
      client.println();
      Serial.print("SavedTime Uploaded: ");
      Serial.println(savedTime);
      place-= 3;
      //Serial.println(savedHours*10000); 
    }
    if (val > -1)
    {
      String a = "POST /test/post.php?temp=";
      client.println(a + val + " HTTP/1.1");
      client.println("Content-Type: application/x-www-form-urlencoded");
      client.print("Content-Length: ");
      client.println(postData.length());
      client.print("Host: ");
      client.println(serveradd);
      client.println();
      client.print(postData);
      client.println();
      Serial.println(postData);
    }   
  }
  else
  {
      if (connected == true)
      {
        connected = false;
        Serial.print("Not Connected\n");
      } 
      if (place < 30)
      {
        int time[3];
        time[2] = val % 100;
        time[1] = ((val - time[2]) % 10000)/100;
        time[0] = ((val - time[2] - time[1])/10000);
        for(int j = 0; j < 3; j++)
        {
          place++;
          EEPROM.write(place, time[j]);
        }
      }
  }

  if (client.connected()) {
    client.stop();
  }
  
  delay(1000);
}

void sensor() {
    detector.loop();
    uint32_t now = millis();
    if (now - last_time > 200) {
        int detects = 0;
              if (detector.presentField1()) detects++;
              if (detector.presentField2()) detects++;
              if (detector.presentField3()) detects++;
              if (detector.presentField4()) detects++;     
        if (detects > 3) {
              timeClient.forceUpdate();
              
              long int testTime = 0;
              long int hours = timeClient.getHours();   // For some reason, this is needed.
              int minutes = timeClient.getMinutes();
              int seconds = timeClient.getSeconds();    
              testTime = (10000*hours) + (100*minutes) + (seconds);
              //Serial.println(testTime);
              time();
              for (int i = 0; i < 3; i++)
              {
                  tone(buzzer, 3000); 
                  delay(1000);        
                  noTone(buzzer);    
                  delay(1000);        
              }   
              webServer(testTime);
              delay(1);     
        }
        last_time = now;
    }
    delay(1);
}

void time() {
  //timeClient.update();
  Serial.print("Triggered: ");
  Serial.print(daysOfTheWeek[timeClient.getDay()]);
  Serial.print(", ");
  Serial.print(timeClient.getHours());
  Serial.print(":");
  Serial.print(timeClient.getMinutes());
  Serial.print(":");
  Serial.println(timeClient.getSeconds());
  //Serial.println(timeClient.getFormattedTime());
  delay(1000);
}
