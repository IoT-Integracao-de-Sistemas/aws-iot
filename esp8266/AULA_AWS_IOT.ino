#include <ESP8266WiFi.h>
#include <WiFiClientSecure.h>
#include <PubSubClient.h>
#include <ArduinoJson.h>
#include <time.h>
#include "DHT.h"

#include "connection_parameters.h"
#include "secrets.h"
 
#define DHTPIN 4        // Digital pin connected to the DHT sensor
#define DHTTYPE DHT11   // DHT 11
 
DHT dht(DHTPIN, DHTTYPE);
 
float h ;
float t;
unsigned long lastMillis = 0;
unsigned long previousMillis = 0;
//const long interval = 5000;
 
WiFiClientSecure secureConnection;
 
BearSSL::X509List cert(AWS_CERT_CA);
BearSSL::X509List client_crt(client_cert);
BearSSL::PrivateKey key(privkey);
 
PubSubClient clientMqtt(secureConnection);
 
time_t now;
time_t nowish = 1510592825;


//********* SETUP  ********* 
void setup()
{
  Serial.begin(115200);

  wifiConnect();
  ntpConnect();
  connectAWS();
  dht.begin();
}


//********* CONEXAO AO WIFI  ********* 
void wifiConnect(){

  WiFi.mode(WIFI_STA);
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
 
  Serial.println(String("CONECTANDO AO SSID: ") + String(WIFI_SSID));
 
  while (WiFi.status() != WL_CONNECTED) {
    Serial.print(".");
    delay(1000);
  }
}


//********* CONEXAO AO NTP  *********
void ntpConnect(void) {
  
  Serial.println("\nCONFIGURANDO TIMESTAMP POR MEIO DE SNTP");
  
  configTime(TIME_ZONE * 3600, 0 * 3600, "pool.ntp.org", "time.nist.gov");
  
  now = time(nullptr);
  
  while (now < nowish) {
    delay(500);
    Serial.print(".");
    now = time(nullptr);
  }
  
  Serial.println("OBTIDO!");
  
  struct tm timeinfo;
  gmtime_r(&now, &timeinfo);
  Serial.print("Current time: ");
  Serial.print(asctime(&timeinfo));
}

//********* CONEXAO AWS IOT  *********
void connectAWS() {

  secureConnection.setTrustAnchors(&cert);
  secureConnection.setClientRSACert(&client_crt, &key);
 
  clientMqtt.setServer(MQTT_HOST, 8883);
  clientMqtt.setCallback(messageReceived);
 
  Serial.println("\nCONECTANDO A AWS IOT");
 
  while (!clientMqtt.connect(THINGNAME)) {
    Serial.print(".");
    delay(1000);
  }
 
  if (!clientMqtt.connected()) {
    Serial.println("CONEXAO COM A AWS IoT FALHOU!");
    return;
  }
  // Subscribe EM UM TÓPICO
  clientMqtt.subscribe(AWS_IOT_SUBSCRIBE_TOPIC);
 
  Serial.println("AWS IoT CONECTADA!");
}
 
void messageReceived(char *topic, byte *payload, unsigned int length) {
  
  Serial.print("RECEBIDO [");
  Serial.print(topic);
  Serial.print("]: ");
  
  for (int i = 0; i < length; i++) {
    Serial.print((char)payload[i]);
  }
  
  Serial.println();
}
 
void publishMessage() {
  
  StaticJsonDocument<200> doc;
  
  doc["time"]        = millis();
  doc["humidity"]    = h;
  doc["temperature"] = t;
  
  char jsonData[512];
  
  serializeJson(doc, jsonData);
 
  clientMqtt.publish(AWS_IOT_PUBLISH_TOPIC, jsonData);
}
 
 
void loop()
{
  h = 80; //dht.readHumidity();
  t = 18; //dht.readTemperature();

  // Verifica se alguma leitura falhou e sai (para tentar novamente).
  if (isnan(h) || isnan(t) ) {
    
    Serial.println("FALHA COM A LEITURA DO SENSOR DHT!");
    return;
  
  }
 
  Serial.print(F("UMIDADE: "));
  Serial.print(h);
  Serial.print(F("%  TEMPERATURA: "));
  Serial.print(t);
  Serial.println(F("°C "));
  
  delay(10000); //TEMPO ENTRE UM PUBLISH E OUTRO
    
  now = time(nullptr);
 
  if (!clientMqtt.connected()) {
    connectAWS();
  } else {
    
    clientMqtt.loop();
    
    if (millis() - lastMillis > 5000) {
      lastMillis = millis();
      publishMessage();
    }
  }
}
