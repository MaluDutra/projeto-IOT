#include <Arduino.h>
#include <Adafruit_BME680.h>
#include <Adafruit_CCS811.h>
#include <WiFi.h> 
#include <WiFiClientSecure.h> 
#include "certificados.h" 
#include <MQTT.h>

// TODO tudo do display

#define BUZZZER_PIN  7

/*=================== USER AREA ========================*/
String id_dispositivo = "1"; // para qualidade do ar
String sala = "LET";         // para temperatura
/*=================== USER AREA ========================*/

WiFiClientSecure conexaoSegura; 
MQTTClient mqtt(1000);

Adafruit_BME680 sensorBME;
Adafruit_CCS811 sensorCCS;

float eCO2 = 0.0; 
float TVOC = 0.0;

unsigned long sampleTime = 10000;
unsigned long instanteAnterior = 0;

// DSM501A
const int pinPM25 = 4;
const int pinPM1 = 5;

volatile unsigned long lowPulseTimePM25 = 0;
volatile unsigned long lastTimePM25 = 0;
volatile unsigned long lowPulseTimePM1 = 0; 
volatile unsigned long lastTimePM1 = 0;

float calc_low_ratio(float lowPulse) {
  return lowPulse / sampleTime * 100.0;  // low ratio in %
}

float calc_c_mgm3(float lowPulse) {
  float r = calc_low_ratio(lowPulse);
  float c_mgm3 = 0.00258425 * pow(r, 2) + 0.0858521 * r - 0.01345549;
  return max(0.0f, c_mgm3);
}

float calc_c_pcs283ml(float lowPulse) {
  float r = calc_low_ratio(lowPulse);
  float c_pcs283ml =  625 * r;
  return min(c_pcs283ml, 12500.0f);
}

void apitaBuzzer()
{
  analogWrite(BUZZZER_PIN, 32);
  delay(100);
  analogWrite(BUZZZER_PIN, 0);
}

// --- Funções de Interrupção (ISR) ---

// ISR para o canal PM2.5 (>2.5µm)
void IRAM_ATTR isrPM25() {
  unsigned long now = micros();
  // Se o pino for LOW, armazena o tempo atual.
  if (digitalRead(pinPM25) == LOW) {
    lastTimePM25 = now;
  } 
  // Se o pino for HIGH, o pulso LOW terminou.
  else {
    // Adiciona a duração do pulso LOW (em microssegundos)
    if (lastTimePM25 != 0) {
      lowPulseTimePM25 += now - lastTimePM25;
      lastTimePM25 = 0;
    }
  }
}

// ISR para o canal PM1.0 (>1.0µm) - Adicionado
void IRAM_ATTR isrPM1() {
  unsigned long now = micros();
  if (digitalRead(pinPM1) == LOW) {
    lastTimePM1 = now;
  } else {
    if (lastTimePM1 != 0) {
      lowPulseTimePM1 += now - lastTimePM1;
      lastTimePM1 = 0;
    }
  }
}

void reconectarWiFi() { 
  if (WiFi.status() != WL_CONNECTED) { 
    WiFi.begin("NOME DA REDE", "SENHA DA REDE"); 
    Serial.print("Conectando ao WiFi..."); 
    while (WiFi.status() != WL_CONNECTED) { 
      Serial.print("."); 
      delay(1000); 
    } 
    Serial.print("conectado!\nEndereço IP: "); 
    Serial.println(WiFi.localIP()); 
  } 
}

void reconectarMQTT() { 
  if (!mqtt.connected()) { 
    Serial.print("Conectando MQTT..."); 
    while(!mqtt.connected()) { 
      mqtt.connect("SEU ID", "LOGIN", "SENHA"); 
      Serial.print("."); 
      delay(1000); 
    } 
    Serial.println(" conectado!"); 
    
    /*
    mqtt.subscribe("dispositivo/" + id_dispositivo);
    mqtt.subscribe("sala/" + sala);
    */
  } 
}

void setup() 
{
  Serial.begin(115200); delay(500);
  delay(10000);
  Serial.println("Projeto IOT - Parte Bento");



  pinMode(BUZZZER_PIN, OUTPUT);

  if (!sensorBME.begin()) 
  { 
    Serial.println("Erro no sensor BME"); 
    while (true)
    {
      sleep(1);
      Serial.println("Erro BME");
    }
  } 

  if (!sensorCCS.begin()) 
  { 
    Serial.println("Erro no sensor CCS"); 
    while (true)
    {
      sleep(1);
      Serial.println("Erro CCS");
    }
  } 

  // iniciado o DSM501
  pinMode(pinPM25, INPUT);
  pinMode(pinPM1, INPUT);

  // Anexa a interrupção ao pino PM2.5, disparando em QUALQUER MUDANÇA (CHANGE)
  attachInterrupt(digitalPinToInterrupt(pinPM25), isrPM25, CHANGE);
  // Anexa a interrupção ao pino PM1.0
  attachInterrupt(digitalPinToInterrupt(pinPM1), isrPM1, CHANGE);

  // Calibrando BME
  // aumenta amostragem dos sensores (1X, 2X, 4X, 8X, 16X ou NONE) 
  sensorBME.setTemperatureOversampling(BME680_OS_8X); 
  sensorBME.setHumidityOversampling(BME680_OS_2X); 
  sensorBME.setGasHeater(320, 150); // Adiciona aquecimento para o sensor de gás

  // Calibrando CCS
  float temp = sensorCCS.calculateTemperature();
  sensorCCS.setTempOffset(temp - 25.0);

  // WIFI e MQTT
  reconectarWiFi(); 
  conexaoSegura.setCACert(certificado1); 
  mqtt.begin("mqtt.janks.dev.br", 1883, conexaoSegura); 
  // mqtt.onMessage(recebeuMensagem); 
  mqtt.setKeepAlive(10); 
  mqtt.setWill("tópico da desconexão", "conteúdo");
  reconectarMQTT();
}

void loop() 
{
  reconectarWiFi(); 
  reconectarMQTT();
  unsigned long instanteAtual = millis();

  //static float lowPM = 0;
  //lowPM += pulseIn(pinPM25, LOW) / 1000.0;   // >2.5µm (PM2.5)

  if (instanteAtual > instanteAnterior + sampleTime)
  {
    sensorBME.performReading();
  
    if (sensorCCS.available() && !sensorCCS.readData())
    {
      eCO2 = sensorCCS.geteCO2();
      TVOC = sensorCCS.getTVOC();

      sensorCCS.setEnvironmentalData(sensorBME.humidity, sensorBME.temperature);
    }

    float temperatura = sensorBME.temperature; 
    float umidade = sensorBME.humidity;

    // --- Processamento do DSM501A ---
    
    // 1. Desabilita as interrupções para garantir que a leitura seja atômica
    //    e que 'lowPulseTime' não seja alterado durante a cópia/reset.
    detachInterrupt(digitalPinToInterrupt(pinPM25));
    detachInterrupt(digitalPinToInterrupt(pinPM1));

    // 2. Copia o tempo acumulado (agora em microssegundos)
    // Converte para milissegundos (dividir por 1000.0) para usar nas suas funções de cálculo.
    float lowPM25_ms = lowPulseTimePM25 / 1000.0;
    float lowPM1_ms = lowPulseTimePM1 / 1000.0;

    // 3. Reseta os contadores para o próximo período de amostragem
    lowPulseTimePM25 = 0;
    lowPulseTimePM1 = 0;

    // 4. Reabilita as interrupções
    attachInterrupt(digitalPinToInterrupt(pinPM25), isrPM25, CHANGE);
    attachInterrupt(digitalPinToInterrupt(pinPM1), isrPM1, CHANGE);

    // --- Cálculos de PM2.5 ---
    float low_percent_PM25 = calc_low_ratio(lowPM25_ms);
    float c_mgm3_PM25 = calc_c_mgm3(lowPM25_ms);
    float c_pcs283ml_PM25 = calc_c_pcs283ml(lowPM25_ms);

    // --- Cálculos de PM1.0 (Exemplo - Usando a mesma fórmula, mas deve ser ajustado para PM1.0 se houver) ---
    float low_percent_PM1 = calc_low_ratio(lowPM1_ms);
    float c_mgm3_PM1 = calc_c_mgm3(lowPM1_ms); // Use fórmulas específicas para PM1 se as tiver
    float c_pcs283ml_PM1 = calc_c_pcs283ml(lowPM1_ms); // Use fórmulas específicas para PM1 se as tiver
    
    Serial.printf("temperatura: %.2f, umildade: %.2f, CO2: %.0f, TVOC: %.0f\n", temperatura, umidade, eCO2, TVOC);
    Serial.printf("DSM501A PM2.5:\nLow Ratio: %.0f, mg/m³: %.4f, pcs/0.283L: %.2f\n", low_percent_PM25, c_mgm3_PM25, c_pcs283ml_PM25);
    Serial.printf("DSM501A PM1.0:\nLow Ratio: %.0f, mg/m³: %.4f, pcs/0.283L: %.2f\n\n", low_percent_PM1, c_mgm3_PM1, c_pcs283ml_PM1);
    instanteAnterior = instanteAtual; 

    // TODO fazer métrica de qualidade do ar
    String QualidadeDoAr = "Bom";

    mqtt.publish("dispositivo/" + id_dispositivo, QualidadeDoAr);
    mqtt.publish("sala/" + sala, String(temperatura));
    

    //apitaBuzzer();
  }
 
  mqtt.loop();
}
