#include <Arduino.h>
#include <Adafruit_BME680.h>
#include <Adafruit_CCS811.h>

Adafruit_BME680 sensorBME;
Adafruit_CCS811 sensorCCS;

float eCO2 = 0.0; 
float TVOC = 0.0;

/*


TODO: o sensor particular (DSM 501A) não possui biblioteca, então terá que ser usado de maneira "crua"


*/

void setup() 
{
  Serial.begin(115200); delay(500);
  Serial.println("Projeto IOT - Parte Bento");

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
  sensorBME.performReading();
  
  if (sensorCCS.available() && !sensorCCS.readData())
  {
    eCO2 = sensorCCS.geteCO2();
    TVOC = sensorCCS.getTVOC();

    sensorCCS.setEnvironmentalData(sensorBME.humidity, sensorBME.temperature);
  }

  float temperatura = sensorBME.temperature; 
  float umidade = sensorBME.humidity;
  Serial.printf("temperatura: %.2f, umildade: %.2f, CO2: %.0f, TVOC: %.0f\n\n", temperatura, umidade, eCO2, TVOC);
  
  delay(10000);
}
