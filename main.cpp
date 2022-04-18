#include <Arduino.h>
#include <Adafruit_ADS1X15.h>
#include <Wire.h>
#include "Secrets.h"
#include <WiFi.h>
#include <PubSubClient.h>
#include <ArduinoJson.h>
#include "WiFi.h"

//MQTT Home assistant
String client_id;
const char *mqtt_broker = "192.168.3.6";
  //Topics
      /*Publish*/
const char *power_pub_topic = "/iot/e2/power/pub"; 
const char *power_pub_laptop = "/iot/e2/power/pub/laptop"; 
const char *power_pub_ventilador = "/iot/e2/power/pub/ventilador"; 
      /*Subscribe*/
const char *foco_sub_topic = "/iot/e2/execute/foco"; 
const char *laptop_sub_topic = "/iot/e2/execute/laptop";
const char *ventilador_sub_topic = "/iot/e2/execute/ventilador";
  /* Status */
const char *foco_status_topic = "/iot/e2/status/foco"; 
const char *laptop_status_topic = "/iot/e2/status/laptop";
const char *ventilador_status_topic = "/iot/e2/status/ventilador";
//Username, password and port
const char *mqtt_username = "homeassistant";
const char *mqtt_password = "yae4oothaeGhaengieceecogh2ieghujafienguniovix8eeNg6ieYaegh0ohviz";
const int mqtt_port = 1883; 

 //ADC object
Adafruit_ADS1115 ads;

const float Factor = 30.0;
const float Factor_100 = 100.0;
const float VRMS = 127.0;
const float mult = 0.092857F;

WiFiClient espClient;
PubSubClient client(espClient);
//Functions definitions 
void callback(char* topic, byte* payload, unsigned int length);
void setup_wifi();
void setup_mqtt();
void reconnect();
float getCurrent();
float getCurrent_100();

void setup() {
  Wire.begin();
  Serial.begin(115200);
  Wire.begin(SDA, SCL);
  Serial.print("INPUT PINS:  SDA=");
  Serial.print(SDA);
  Serial.print("   SCL=");
  Serial.println(SCL);
  ads.setGain(GAIN_TWO);
  while(!ads.begin(0x48)) {
    Serial.println("Failed to initialize ADS.");
    delay(500);
  }
  setup_wifi();
  setup_mqtt();
}
 
void loop() {
  byte error, address;
  float Current_L, Power_L;
  float Current_V, Power_V;
  float Current_total, Power_total;
  char buf_p_laptop[16];
  char buf_p_ventilador[16];
  char buf_p_total[16];
  address = 72;

  while (WiFi.status() != WL_CONNECTED)
  {
    delay(500);
    Serial.print("WiFi disconnected");
  }
  if(!client.connected()) {
    reconnect();
  } 
  client.loop();
  Wire.beginTransmission(address);
  error = Wire.endTransmission();
  if (error == 0) {
      Serial.print("I2C device found at address 0x");
      Serial.println(address,HEX);
      Current_L = getCurrent();
      Power_L = VRMS * Current_L;
      Serial.print("\tLaptop current: ");
      Serial.print(Current_L, 4);
      Serial.println(" A ");
      Serial.print("\tLaptop power: ");
      Serial.print(Power_L, 4);
      Serial.println(" W ");
      Serial.println();
      Current_V = getCurrent_100();
      Power_V = VRMS  * Current_V;
      Serial.print("\tFan current: ");
      Serial.print(Current_V, 4);
      Serial.println(" A ");
      Serial.print("\tFan power: ");
      Serial.print(Power_V, 4);
      Serial.println(" W ");
      Serial.println();
      Current_total = Current_L + Current_V;
      Power_total = Power_L + Power_V;
      Serial.print("\tTotal current: ");
      Serial.print(Current_total, 4);
      Serial.println(" A ");
      Serial.print("\tTotal power: ");
      Serial.print(Power_total, 4);
      Serial.println(" W ");
      Serial.println();
      dtostrf(Power_L, 5, 2, buf_p_laptop);
      dtostrf(Power_V, 5, 2, buf_p_ventilador);
      dtostrf(Power_total, 5, 2, buf_p_total);
      client.publish(power_pub_laptop,buf_p_laptop);
      client.publish(power_pub_ventilador,buf_p_ventilador);
      client.publish(power_pub_topic,buf_p_total);
  }
  else if (error==4) {
      Serial.print("Unknow error at address 0x");
      Serial.println(address,HEX);
  }
  else {
    Serial.println("I2C device not found.");    
  }
  delay(1000);          
}

void callback(char* topic, byte* payload, unsigned int length) {
  Serial.print("Message arrived: ");

  Serial.print(topic);
  Serial.print("] ");

  for (int i = 0; i < length; i++) {
    Serial.print((char)payload[i]);
  }
  Serial.println();
  Serial.println("-----------------");
}
void setup_wifi() {
  Serial.print("Connecting to ");
  Serial.println(WIFI_SSID);
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  while (WiFi.status() != WL_CONNECTED) {
  delay(500);Serial.print(".");}
  Serial.println("WiFi connected");
}
void setup_mqtt() {
  client.setServer(mqtt_broker, mqtt_port);
  client.setCallback(callback);
  while(!client.connected()) {
    client_id = "esp32s-client-";
    client_id += String(WiFi.macAddress());
    Serial.printf("The client %s is trying to connect to the public mqtt broker\n", client_id.c_str());
    if(client.connect(client_id.c_str(), mqtt_username, mqtt_password)) {
      Serial.println("MQTT broker connected");
    } else {
      Serial.print("failed with state ");
      Serial.println(client.state());
      delay(2000);
    }
  }
  client.publish(power_pub_topic, "1");
  client.subscribe(foco_sub_topic);
  client.subscribe(laptop_sub_topic);
  client.subscribe(ventilador_sub_topic);
}
void reconnect() {
  while (!client.connected()) {
    Serial.print("Attempting MQTT connection...");
    if (client.connect(client_id.c_str(), mqtt_username, mqtt_password)) {
      Serial.println("Connected");
      client.subscribe(foco_sub_topic);
    } else {
      Serial.print("failed, rc=");
      Serial.print(client.state());
      Serial.println(" try again in 5 seconds");
      // Wait 5 seconds before retrying
      delay(5000);
      }
   }
}
float getCurrent() {
  float V;
  float C;
  float sum = 0;
  long time = millis();
  int counter = 0;

  while(millis() - time < 1000)
  {
    V = ads.readADC_Differential_0_1() * mult;
    //Serial.println(V);
    C = V * Factor;
    C = C / 1000.0;

    sum += sq(C);
    counter ++;
  }
  C = sqrt(sum/counter);
  return(C);
}
float getCurrent_100() {
  float V;
  float C;
  float sum = 0;
  long time = millis();
  int counter = 0;

  while(millis() - time < 1000)
  {
    V = ads.readADC_Differential_2_3() * mult;
    C = V * Factor_100;
    C = C / 1000.0;

    sum += sq(C);
    counter ++;
  }
  C = sqrt(sum/counter);
  return(C);
}
