#include <Arduino.h>
#include "DallasTemperature.h"
#include "OneWire.h"
#include "PubSubClient.h"
#include "WiFi.h"
#include "string.h"
// Homerseklet szenzorok, labkiosztas
#define T1 18
#define T2 19
#define T3 21

bool tog_pin = false;
bool heating;
uint8_t heating_range;
float temperature_out, temperature_in, temperature_water;
char* temp_txt;
String message;
char out_message[100];
// OneWire szenzorok letrehozasa
OneWire oneWire1(T1);
OneWire oneWire2(T2);
OneWire oneWire3(T3);
// DallasTemperature szenzor definialasa
DallasTemperature sensor1(&oneWire1);
DallasTemperature sensor2(&oneWire2);
DallasTemperature sensor3(&oneWire3);
// dioty servere gepen keresztul is fel lehet menni, ott kiir minden adatot ami szukseges
const char* wifiName = "selma-razvoj";              // WiFi nev
const char* wifiPass = "selmasubotica";             // WiFi kod
const char* diotyUser = "tilben97biker@gmail.com";  // dioty-s username, alltalaban az email cimed
const char* diotyPass = "0dc9d70b";                 // kod amit emailbe kuld a dioty
const uint16_t diotyPort = 1883;                    // port alltalaban mindig 1883 
const char* myServer = "mqtt.dioty.co";             // ez nem valtozik
const char* in = "/tilben97biker@gmail.com/in";     // /email_cimed/in
const char* out1 = "/tilben97biker@gmail.com/temp1";   // /email_cimed/temp1
const char* out2 = "/tilben97biker@gmail.com/temp2";  // /email_cimed/temp2
const char* out3 = "/tilben97biker@gmail.com/temp3";  // /email_cimed/temp3
/////////////////////////////////////////////////////////////////////////////////////////////
WiFiClient espClient;
PubSubClient mqttClient(espClient);
// Itt erkezik be az uzenet //////////////////////////////////////////////////////////////////
void mqttCallback(char* topic, byte* payload, unsigned int length)
{
  char py[100];
  memccpy(py, payload, 1 , sizeof(payload));
  if(payload[0] == 't' && payload[1] == 'r' && payload[2] == 'u' && payload[3] == 'e')
  {
    heating = true;
    Serial.println("Heating is on");
  }
  else if (payload[0] == 'f' && payload[1] == 'a' && payload[2] == 'l' && payload[3] == 's' && payload[4] == 'e')
  {
    heating = false;
    Serial.println("Heating is off");
  }
  else
  {
    heating_range = atoi(py);
    Serial.print("Heating is set to the ");
    Serial.print(heating_range);
    Serial.println(" ºC");
  }
  
  
}
////////////////////////////////////////////////////////////////////////////////////////////////
void setup() {
  Serial.begin(9600);
  Serial.println("Setup the sensors...");
  sensor1.begin();
  sensor2.begin();
  sensor3.begin();
  WiFi.begin(wifiName, wifiPass);                 // Csatlakozas a WiFi-re
  Serial.print("Connecting to the WiFi...");
  while(WiFi.status() != WL_CONNECTED)
  {
    Serial.print(".");
    delay(200);
  }
  Serial.print("\nConnected to the WiFi ---->  ");
  Serial.println(WiFi.localIP());
  mqttClient.setServer(myServer, diotyPort);        // Server beallitas
  
  mqttClient.setCallback(mqttCallback);             // Itt allitod be a bejovo uzenet fogadasat...
  Serial.print("Connecting to the MQTT Client...");
  while (!mqttClient.connected())
  {
    Serial.print(".");
    if(mqttClient.connect("ESP32", diotyUser, diotyPass))   // MQTT severe csatlakozas
    {
      Serial.print("\nConnected to MQTT Client");
      mqttClient.subscribe(in);                     // a bejovo uzenet topic-a, feliratkozol ra
    }
  }
  

  
  pinMode(LED_BUILTIN, OUTPUT);
}

void loop() {
  // Sensor ertekek kiolvasasa
  sensor1.requestTemperatures();
  sensor2.requestTemperatures();
  sensor3.requestTemperatures();
  temperature_in = sensor1.getTempCByIndex(0);
  temperature_out = sensor2.getTempCByIndex(0);
  temperature_water = sensor3.getTempCByIndex(0);
////////////////////////////////////////////////////
// Futes iranyitasa
  if(heating)
  {
    if(temperature_in < heating_range)
    {
      digitalWrite(LED_BUILTIN, HIGH);
    }
    else
    {
      digitalWrite(LED_BUILTIN, LOW);
    }
  }
  ////////////////////////////////////////////////////
  // Kiiratas Sorossan
  Serial.print("Inside temperature: ");
  Serial.print(temperature_in);
  Serial.println(" ºC");
  Serial.print("Outside temperature: ");
  Serial.print(temperature_out);
  Serial.println(" ºC");
  Serial.print("Water temperature: ");
  Serial.print(temperature_water);
  Serial.println(" ºC");
  //////////////////////////////////////////////////
  // Uzenet letrehozasa
  message = "";
  message += "Inside temperature: ";
  message += temperature_in;
  message += " ºC";
  // Kuldes mqtt servere
  message.toCharArray(out_message, sizeof(out_message));
  mqttClient.publish(out1, out_message);
  // Uzenet letrehozasa
  message = "";
  message += "Outside temperature: ";
  message += temperature_out;
  message += " ºC\n";
  // Kuldes mqtt servere
  message.toCharArray(out_message, sizeof(out_message));
  mqttClient.publish(out2, out_message);
  // Uzenet letrehozasa
  message = "";
  message += "Water temperature: ";
  message += temperature_water;
  message += " ºC";
  // Kuldes mqtt servere
  message.toCharArray(out_message, sizeof(out_message));
  mqttClient.publish(out3, out_message);
  //MQTT loop
  mqttClient.loop();
  // 100 ms befagyasztas
  delay(100);
}