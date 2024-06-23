#include <LiquidCrystal.h>
#include "DHT.h"

#include <WiFi.h>
#include <WiFiClient.h>
#include <PubSubClient.h>

#define DHTTYPE DHT11
#define INTERNAL_LED 2
#define NUM_READINGS 100

const int dht_pin = 4;
const int lcd_pin_rs = 21;
const int lcd_pin_en = 19;
const int lcd_pin_d4 = 5;
const int lcd_pin_d5 = 18;
const int lcd_pin_d6 = 22;
const int lcd_pin_d7 = 23;
LiquidCrystal lcd(lcd_pin_rs, lcd_pin_en, lcd_pin_d4, lcd_pin_d5, lcd_pin_d6, lcd_pin_d7);
DHT DHT(dht_pin, DHTTYPE);

WiFiClient wifi_client;
PubSubClient mqtt_client(wifi_client);

const char* wifi_ssid = "Ernane 2.4";
const char* wifi_password = "19202119";
int wifi_timeout = 100000;

const char* mqtt_broker = "io.adafruit.com";
const int mqtt_port = 1883;
int mqtt_timeout = 10000;

const char* mqtt_usernameAdafruitIO = "Ernane";
const char* mqtt_keyAdafruitIO = "aio_nQOd21pewunds4UW1YjgbkKEEpBe";


float hum = 0.0;
float temp = 0.0;
float averageTemp = 0.0;
float averageHum = 0.0;
float tempReadings[NUM_READINGS];
float humReadings[NUM_READINGS];
int readIndex = 0;

void setup() {
  pinMode(dht_pin, INPUT);
  pinMode(INTERNAL_LED,OUTPUT);

  Serial.begin(115200);
  lcd.begin(16,2);

  lcd.print("      IACIM                               Iniciando...  ");
  DHT.begin();
  
  WiFi.mode(WIFI_STA); // "station mode": permite o ESP32 ser um cliente da rede WiFi
  WiFi.begin(wifi_ssid, wifi_password);
  connectWiFi();
  mqtt_client.setServer(mqtt_broker, mqtt_port);

  lcd.clear();
  lcd.print("      IACIM                                 Iniciado!   ");
  delay(1000);
  lcd.clear();

  for (int i = 0; i < NUM_READINGS; i++) {
    tempReadings[i] = 0;
    humReadings[i] = 0;
  }
}

void loop() {
  Serial.println("Capturando dados..");
  temp = DHT.readTemperature();
  hum = DHT.readHumidity();
  tempReadings[readIndex] = temp;
  humReadings[readIndex] = hum;

  averageTemp = 0.0;
  averageHum = 0.0;
  int countTemps = 0;
  int countHum = 0;
  for (int i = 0; i < NUM_READINGS; i++) {
    averageTemp += tempReadings[i];
    if(tempReadings[i] != 0.0){
      countTemps++;
    }
    averageHum += humReadings[i];
    if(humReadings[i] != 0.0){
      countHum++;
    }
  }

  readIndex = (readIndex + 1) % NUM_READINGS;

  averageTemp = averageTemp / countTemps;
  averageHum = averageHum / countHum;
  Serial.println("Dados capturados!");

  Serial.println("Escrevendo no LCD..");
  writeHTToLCD();
  Serial.println("LCD atualiado!");

  connectMQTT(); // TEMP
  if (mqtt_client.connected()) {
    Serial.println("MQTT conectado!");
    mqtt_client.loop();
    
    Serial.println("Enviando Temperatura");
    if (mqtt_client.publish("Ernane/feeds/actm-temp", String(temp).c_str(), true)) {
      Serial.println("Temperatura enviada com sucesso");
    } else {
      Serial.println("Falha ao enviar Temperatura");
    }
  } else {
    Serial.println("MQTT não conectado. Tentando conectar..");
    connectMQTT();
    return;
  }

  delay(5000);

  connectMQTT(); // Average TEMP
  if (mqtt_client.connected()) {
    Serial.println("MQTT conectado!");
    mqtt_client.loop();
    
    Serial.println("Enviando Temperatura media");
    if (mqtt_client.publish("Ernane/feeds/actm-atemp", String(averageTemp).c_str(), true)) {
      Serial.println("Temperatura media enviada com sucesso");
    } else {
      Serial.println("Falha ao enviar media da Temperatura");
    }
  } else {
    Serial.println("MQTT não conectado. Tentando conectar..");
    connectMQTT();
    return;
  }

  delay(5000);

  connectMQTT(); // HUM
  if (mqtt_client.connected()) {
    Serial.println("Enviando Umidade");
    if (mqtt_client.publish("Ernane/feeds/actm-hum", String(hum).c_str(), true)) {
      Serial.println("Umidade enviada com sucesso");
    } else {
      Serial.println("Falha ao enviar Umidade");
    }
  } else {
    Serial.println("MQTT não conectado. Tentando conectar..");
    connectMQTT();
    return;
  }

  delay(5000);

  connectMQTT(); // Average HUM
  if (mqtt_client.connected()) {
    Serial.println("MQTT conectado!");
    mqtt_client.loop();
    
    Serial.println("Enviando Umidade Media");
    if (mqtt_client.publish("Ernane/feeds/actm-ahum", String(averageHum).c_str(), true)) {
      Serial.println("Umidade media enviada com sucesso");
    } else {
      Serial.println("Falha ao enviar media da Umidade");
    }
  } else {
    Serial.println("MQTT não conectado. Tentando conectar..");
    connectMQTT();
    return;
  }

  delay(30000);
}

void connectWiFi() {
  Serial.println("Conectando à rede WiFi .. ");

  unsigned long tempoInicial = millis();
  while (WiFi.status() != WL_CONNECTED && (millis() - tempoInicial < wifi_timeout)) {
    Serial.print(".");
    delay(100);

    digitalWrite(INTERNAL_LED,HIGH);
    delay(50);
    digitalWrite(INTERNAL_LED,LOW);
    delay(50);
  }
  Serial.println();

  if (WiFi.status() != WL_CONNECTED) {
    Serial.println("Conexão com WiFi falhou!");
    digitalWrite(INTERNAL_LED,LOW);
  } else {
    Serial.print("Conectado com o IP: ");
    Serial.println(WiFi.localIP());
    digitalWrite(INTERNAL_LED,HIGH);
  }
}

void connectMQTT() {
  unsigned long tempoInicial = millis();
  while (!mqtt_client.connected() && (millis() - tempoInicial < mqtt_timeout)) {
    if (WiFi.status() != WL_CONNECTED) {
      connectWiFi();
    }
    Serial.println("Conectando ao MQTT Broker..");
    
    if (mqtt_client.connect("ESP32Client", mqtt_usernameAdafruitIO, mqtt_keyAdafruitIO)) {
      Serial.println("Conectado ao broker MQTT!");
    } else {
      Serial.println("Conexão com o broker MQTT falhou!");
      Serial.print("State: ");
      Serial.println(mqtt_client.state());
    }
    delay(100);
  }
}

void writeHTToLCD() {
  lcd.clear();

  lcd.setCursor(0, 0);
  lcd.print("T:");
  lcd.print(temp, 1);
  lcd.print("C|");

  lcd.print("aT:");
  lcd.print(averageTemp, 1);
  lcd.print("C");

  lcd.setCursor(0, 1);
  lcd.print("H:");
  lcd.print(hum, 1);
  lcd.print("%|");

  lcd.print("aH:");
  lcd.print(averageHum, 1);
  lcd.print("%");

  delay(3000); // Atraso para permitir a leitura das informações
}

