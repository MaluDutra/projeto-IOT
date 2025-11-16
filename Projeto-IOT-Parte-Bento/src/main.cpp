#include <Arduino.h>
#include <Adafruit_BME680.h>
#include <Adafruit_CCS811.h>

#define BUZZZER_PIN  7

Adafruit_BME680 sensorBME;
Adafruit_CCS811 sensorCCS;

float eCO2 = 0.0; 
float TVOC = 0.0;

unsigned long sampleTime = 10000;
unsigned long instanteAnterior = 0;

// DSM501A
const int pinPM25 = 4;
const int pinPM1 = 5;

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

void setup() 
{
  Serial.begin(115200); delay(500);
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

  // Calibrando BME
  // aumenta amostragem dos sensores (1X, 2X, 4X, 8X, 16X ou NONE) 
  sensorBME.setTemperatureOversampling(BME680_OS_8X); 
  sensorBME.setHumidityOversampling(BME680_OS_2X); 
  sensorBME.setGasHeater(320, 150); // Adiciona aquecimento para o sensor de gás

  // Calibrando CCS
  float temp = sensorCCS.calculateTemperature();
  sensorCCS.setTempOffset(temp - 25.0);

}

void loop() 
{
  unsigned long instanteAtual = millis();

  static float lowPM = 0;
  lowPM += pulseIn(pinPM25, LOW) / 1000.0;   // >2.5µm (PM2.5)

  if (instanteAtual > instanteAnterior + sampleTime)
  {
    digitalWrite(BUZZZER_PIN, HIGH);
    sensorBME.performReading();
  
    if (sensorCCS.available() && !sensorCCS.readData())
    {
      eCO2 = sensorCCS.geteCO2();
      TVOC = sensorCCS.getTVOC();

      sensorCCS.setEnvironmentalData(sensorBME.humidity, sensorBME.temperature);
    }

    float temperatura = sensorBME.temperature; 
    float umidade = sensorBME.humidity;

    //START DSM501
    float low_percent = calc_low_ratio(lowPM);
    float c_mgm3 = calc_c_mgm3(lowPM);
    float c_pcs283ml = calc_c_pcs283ml(lowPM);

    lowPM = 0;
    // END
    
    Serial.printf("temperatura: %.2f, umildade: %.2f, CO2: %.0f, TVOC: %.0f, low_percent: %f, c_mg3: %f, c_pcs283ml: %f \n\n", temperatura, umidade, eCO2, TVOC, low_percent, c_mgm3, c_pcs283ml);
    instanteAnterior = instanteAtual; 
    //apitaBuzzer();
  }
}
