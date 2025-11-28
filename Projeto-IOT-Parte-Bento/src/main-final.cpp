#include <Arduino.h>
#include <Adafruit_BME680.h>
#include <Adafruit_CCS811.h>
#include <WiFi.h>
#include <WiFiClientSecure.h>
#include "certificados.h"
#include <MQTT.h>
#include <GxEPD2_BW.h>
#include <U8g2_for_Adafruit_GFX.h>
#include <ArduinoJson.h>
#include <IRremoteESP8266.h>
#include <IRac.h>
#include <IRutils.h>
#include <string>

#define BUZZZER_PIN 1

/*=================== área do usuário ========================*/
int num_dispositivo = 1; // para qualidade do ar
String sala = "LET";     // para temperatura
/*=================== área do usuário ========================*/

U8G2_FOR_ADAFRUIT_GFX fontes;
GxEPD2_290_T94_V2 modeloTela(10, 14, 15, 16);
GxEPD2_BW<GxEPD2_290_T94_V2, GxEPD2_290_T94_V2::HEIGHT> tela(modeloTela);

WiFiClientSecure conexaoSegura;
MQTTClient mqtt(1000);

const uint16_t IRled = 4;    // pino do bagulho que envia
IRac ar_condicionado(IRled); // classe que realiza o controle genérico do ar condicionado

Adafruit_BME680 sensorBME;
Adafruit_CCS811 sensorCCS;

float eCO2 = 0.0;
float TVOC = 0.0;

unsigned long sampleTime = 10000;

// DSM501A
const int pinPM25 = 4;
const int pinPM1 = 5;

volatile unsigned long lowPulseTimePM25 = 0;
volatile unsigned long lastTimePM25 = 0;
volatile unsigned long lowPulseTimePM1 = 0;
volatile unsigned long lastTimePM1 = 0;

enum class comando
{
  Power,
  Temperature,
  OpMode,
  FanSpeed
};

union tiposDeInstrucoes
{
  bool ligar;
  float temperatura;
  stdAc::opmode_t modo_operacao;
  stdAc::fanspeed_t vel_ventilador;
};

struct ACControle
{
  comando tipo;
  tiposDeInstrucoes instrucao;
};

stdAc::state_t estadoDoAr;

float calc_low_ratio(float lowPulse)
{
  return lowPulse / sampleTime * 100.0; // low ratio in %
}

float calc_c_mgm3(float lowPulse)
{
  float r = calc_low_ratio(lowPulse);
  float c_mgm3 = 0.00258425 * pow(r, 2) + 0.0858521 * r - 0.01345549;
  return max(0.0f, c_mgm3);
}

float calc_c_pcs283ml(float lowPulse)
{
  float r = calc_low_ratio(lowPulse);
  float c_pcs283ml = 625 * r;
  return min(c_pcs283ml, 12500.0f);
}

void apitaBuzzer()
{
  analogWrite(BUZZZER_PIN, 32);
  delay(150);
  analogWrite(BUZZZER_PIN, 0);
}

// --- Funções de Controle do Ar ---

void setup_AC()
{
  ar_condicionado.next.protocol = decode_type_t::COOLIX; // Set a protocol to use.
  ar_condicionado.next.model = 1;                        // Some A/Cs have different models. Try just the first.
  ar_condicionado.next.mode = static_cast<stdAc::opmode_t>(1);
  ar_condicionado.next.celsius = true;  // Use Celsius for temp units. False = Fahrenheit
  ar_condicionado.next.degrees = 25.0f; // 25 degrees.
  ar_condicionado.next.fanspeed = static_cast<stdAc::fanspeed_t>(3);
  ar_condicionado.next.swingv = stdAc::swingv_t::kOff; // Don't swing the fan up or down.
  ar_condicionado.next.swingh = stdAc::swingh_t::kOff; // Don't swing the fan left or right.
  ar_condicionado.next.light = false;                  // Turn off any LED/Lights/Display that we can.
  ar_condicionado.next.beep = false;                   // Turn off any beep from the A/C if we can.
  ar_condicionado.next.econo = false;                  // Turn off any economy modes if we can.
  ar_condicionado.next.filter = false;                 // Turn off any Ion/Mold/Health filters if we can.
  ar_condicionado.next.turbo = false;                  // Don't use any turbo/powerful/etc modes.
  ar_condicionado.next.quiet = false;                  // Don't use any quiet/silent/etc modes.
  ar_condicionado.next.sleep = -1;                     // Don't set any sleep time or modes.
  ar_condicionado.next.clean = false;                  // Turn off any Cleaning options if we can.
  ar_condicionado.next.clock = -1;                     // Don't set any current time if we can avoid it.
  ar_condicionado.next.power = true;                   // Initially start with the unit on
}

void envia_comando(ACControle *comando)
{
  decode_type_t protocolo = COOLIX;
  if (ar_condicionado.isProtocolSupported(protocolo))
  {
    stdAc::state_t estado = ar_condicionado.next;
    estado.protocol = protocolo;

    switch (comando->tipo)
    {
    case comando::Power:
      estado.power = comando->instrucao.ligar;
      break;
    case comando::OpMode:
      estado.mode = comando->instrucao.modo_operacao;
      break;
    case comando::Temperature:
      estado.degrees = comando->instrucao.temperatura;
      break;
    case comando::FanSpeed:
      estado.fanspeed = comando->instrucao.vel_ventilador;
      break;
    default:
      Serial.println("Comando invalido!");
      break;
    }

    Serial.println("Enviando comando a um ar condicionado " + typeToString(protocolo));
    ar_condicionado.next = estado; // The state we want the device to be in after we send.
    ar_condicionado.sendAc();      // Send an A/C message based soley on our internal state
  }
}

void imprimeEstadoDoAr(stdAc::state_t estado)
{
  // Temperatura do ar
  Serial.println("Temperatura do ar = " + String(estado.degrees, 1U) + " °C");

  // Modo do ar
  switch (estado.mode)
  {
    using namespace stdAc;
  case opmode_t::kAuto:
    Serial.println("AUTO");
    break;
  case opmode_t::kCool:
    Serial.println("COOL");
    break;
  case opmode_t::kDry:
    Serial.println("SECAR");
    break;
  case opmode_t::kFan:
    Serial.println("VENTILAR");
    break;
  case opmode_t::kHeat:
    Serial.println("AQUECER");
    break;
  case opmode_t::kOff:
    Serial.println("DESLIGADO");
    break;
  }

  // Velocidade do ventilador
  Serial.println("Velocidade do ar = " + String(static_cast<int>(estado.fanspeed)));

  // Power
  if (estado.power)
  {
    Serial.println("LIGADO");
  }
  else
  {
    Serial.println("DESLIGADO");
  }
}

// --- Funções de Interrupção (ISR) ---

// ISR para o canal PM2.5 (>2.5µm)
void IRAM_ATTR isrPM25()
{
  unsigned long now = micros();
  // Se o pino for LOW, armazena o tempo atual.
  if (digitalRead(pinPM25) == LOW)
  {
    lastTimePM25 = now;
  }
  // Se o pino for HIGH, o pulso LOW terminou.
  else
  {
    // Adiciona a duração do pulso LOW (em microssegundos)
    if (lastTimePM25 != 0)
    {
      lowPulseTimePM25 += now - lastTimePM25;
      lastTimePM25 = 0;
    }
  }
}

// ISR para o canal PM1.0 (>1.0µm) - Adicionado
void IRAM_ATTR isrPM1()
{
  unsigned long now = micros();
  if (digitalRead(pinPM1) == LOW)
  {
    lastTimePM1 = now;
  }
  else
  {
    if (lastTimePM1 != 0)
    {
      lowPulseTimePM1 += now - lastTimePM1;
      lastTimePM1 = 0;
    }
  }
}

void reconectarWiFi()
{
  if (WiFi.status() != WL_CONNECTED)
  {
    WiFi.begin("Projeto", "2022-11-07");
    Serial.print("Conectando ao WiFi...");
    while (WiFi.status() != WL_CONNECTED)
    {
      Serial.print(".");
      delay(1000);
    }
    Serial.print("conectado!\nEndereço IP: ");
    Serial.println(WiFi.localIP());
  }
}

void reconectarMQTT()
{
  if (!mqtt.connected())
  {
    Serial.print("Conectando MQTT...");
    while (!mqtt.connected())
    {
      mqtt.connect("projeto_arcondicionado", "aula", "zowmad-tavQez");
      Serial.print(".");
      delay(1000);
    }
    Serial.println(" conectado!");

    mqtt.subscribe("arcondicionado/dispositivos/" + String(num_dispositivo), 1);
  }
}

void recebeuMensagem(String topico, String conteudo)
{
  Serial.println(topico + ": " + conteudo);

  if (topico.startsWith("arcondicionado/dispositivos/" + String(num_dispositivo)))
  {
    // recebeu mudança de temperatura!
    Serial.print("Mudar para temperatura: ");
    Serial.println(conteudo);

    ACControle comandoDoAr;
    comandoDoAr.tipo = comando::Temperature;
    float nova_temperatura = conteudo.toFloat();
    comandoDoAr.instrucao.temperatura = nova_temperatura;

    envia_comando(&comandoDoAr);

    Serial.println("Novo estado do ar:");
    estadoDoAr = ar_condicionado.getState(); // Para pegar o estado do ar
    imprimeEstadoDoAr(estadoDoAr);
    Serial.println("==============================================\n");
  }
}

void desenhaEsqueleto()
{
  tela.fillScreen(GxEPD_WHITE);

  fontes.setFont(u8g2_font_open_iconic_all_4x_t);
  fontes.setFontMode(1);
  fontes.setCursor(10, 60);
  fontes.print((char)141);
  fontes.setCursor(10, 110);
  fontes.print((char)152);

  tela.drawCircle(230, 75, 40, GxEPD_BLACK);
  tela.fillCircle(210, 60, 4, GxEPD_BLACK);
  tela.fillCircle(250, 60, 4, GxEPD_BLACK);

  fontes.setFont(u8g2_font_helvB10_te);
  fontes.setFontMode(1);
  fontes.setCursor(170, 20);
  fontes.print("Qualidade do ar:");
  tela.display(true);

  Serial.println("Esqueleto desenhado!");
}

void desenhaDados(float temperatura, float umidade, int qualidadeDoAr)
{
  char aux[10];
  snprintf(aux, sizeof(aux), "%.1f", temperatura);
  String tempString = String(aux) + " °C";

  snprintf(aux, sizeof(aux), "%.1f", umidade);
  String umiString = String(aux) + " %";

  fontes.setFont(u8g2_font_helvB10_te);
  fontes.setFontMode(1);
  fontes.setCursor(50, 55);
  fontes.print(tempString);

  fontes.setCursor(50, 105);
  fontes.print(umiString);

  if (qualidadeDoAr == 0) // ruim
  {
    tela.drawCircle(230, 95, 15, GxEPD_BLACK);
    tela.fillRect(212, 90, 37, 21, GxEPD_WHITE);
  }
  else if (qualidadeDoAr == 1) // média
  {
    tela.drawLine(220, 90, 240, 90, GxEPD_BLACK);
  }
  else // boa
  {
    tela.drawCircle(230, 95, 15, GxEPD_BLACK);
    tela.fillRect(210, 70, 40, 30, GxEPD_WHITE);
  }

  Serial.println("Desenhando dados!");
  tela.display(true);
}

int calculaQualidadeDoAr(float co2, float tvoc, float pm25_mgm3, float pm1_mgm3)
{
  // A variável 'iqa' começará no nível Bom (2) e será rebaixada se necessário.
  int iqa = 2; // 2 = Bom

  // --- Limites dos Poluentes (Mantidos da implementação anterior) ---

  // eCO2: Ruim > 1000, Médio 801-1000
  if (co2 > 1000.0)
  {
    iqa = 0;
  }
  else if (co2 > 800.0 && iqa > 200.0)
  {
    iqa = 1;
  }

  // TVOC: Ruim > 500, Médio 251-500
  if (tvoc > 500.0)
  {
    iqa = 0;
  }
  else if (tvoc > 250.0 && iqa > 0)
  {
    iqa = min(iqa, 1);
  }

  // PM2.5 (em mg/m³): Ruim > 0.060, Médio 0.031-0.060
  if (pm25_mgm3 > 0.060)
  {
    iqa = 0;
  }
  else if (pm25_mgm3 > 0.030 && iqa > 0.010)
  {
    iqa = min(iqa, 1);
  }

  // --- Novos Limites de Conforto/Poluentes ---
  // PM1.0 (Convertido para µg/m³): Ruim > 35, Médio 16-35
  float pm1_ugm3 = pm1_mgm3 * 1000.0;
  if (pm1_ugm3 > 35.0)
  {
    iqa = min(iqa, 0); // Ruim
  }
  else if (pm1_ugm3 > 15.0 && iqa > 5.0)
  {
    iqa = min(iqa, 1); // Médio
  }

  return iqa;
}

void setup()
{
  Serial.begin(115200);
  delay(500);
  Serial.println("Projeto IOT - Ar Condicionado");

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

  // Tela
  tela.init();
  tela.setRotation(3);
  tela.fillScreen(GxEPD_WHITE);
  tela.display(true);

  fontes.begin(tela);
  fontes.setForegroundColor(GxEPD_BLACK);

  desenhaEsqueleto();

  // WIFI e MQTT
  reconectarWiFi();
  conexaoSegura.setCACert(certificado1);

  mqtt.begin("mqtt.janks.dev.br", 8883, conexaoSegura);
  mqtt.onMessage(recebeuMensagem);
  mqtt.setKeepAlive(10);

  char topico_will[] = "arcondicionado/dispositivos/desconexao";
  char mensagem_will[] = "1"; // num_dispositivo
  mqtt.setWill(topico_will, mensagem_will);
  reconectarMQTT();

  // Controle do Ar
  setup_AC();
  Serial.println("Estado inicial do ar:");
  estadoDoAr = ar_condicionado.next;
  imprimeEstadoDoAr(estadoDoAr);
  Serial.println("==============================================\n");
}

unsigned long instanteAnterior = 0;
void loop()
{
  reconectarWiFi();
  reconectarMQTT();
  mqtt.loop();

  unsigned long instanteAtual = millis();

  // static float lowPM = 0;
  // lowPM += pulseIn(pinPM25, LOW) / 1000.0;   // >2.5µm (PM2.5)

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
    detachInterrupt(digitalPinToInterrupt(pinPM25));
    detachInterrupt(digitalPinToInterrupt(pinPM1));

    float lowPM25_ms = lowPulseTimePM25 / 1000.0;
    float lowPM1_ms = lowPulseTimePM1 / 1000.0;

    lowPulseTimePM25 = 0;
    lowPulseTimePM1 = 0;

    attachInterrupt(digitalPinToInterrupt(pinPM25), isrPM25, CHANGE);
    attachInterrupt(digitalPinToInterrupt(pinPM1), isrPM1, CHANGE);

    // --- Cálculos de PM2.5 ---
    float low_percent_PM25 = calc_low_ratio(lowPM25_ms);
    float c_mgm3_PM25 = calc_c_mgm3(lowPM25_ms);
    float c_pcs283ml_PM25 = calc_c_pcs283ml(lowPM25_ms);

    // --- Cálculos de PM1.0 ---
    float low_percent_PM1 = calc_low_ratio(lowPM1_ms);
    float c_mgm3_PM1 = calc_c_mgm3(lowPM1_ms);
    float c_pcs283ml_PM1 = calc_c_pcs283ml(lowPM1_ms);

    Serial.printf("temperatura: %.2f, umildade: %.2f, CO2: %.0f, TVOC: %.0f\n", temperatura, umidade, eCO2, TVOC);
    Serial.printf("DSM501A PM2.5:\nLow Ratio: %.0f, mg/m³: %.4f, pcs/0.283L: %.2f\n", low_percent_PM25, c_mgm3_PM25, c_pcs283ml_PM25);
    Serial.printf("DSM501A PM1.0:\nLow Ratio: %.0f, mg/m³: %.4f, pcs/0.283L: %.2f\n\n", low_percent_PM1, c_mgm3_PM1, c_pcs283ml_PM1);

    int qualidadeDoAr = calculaQualidadeDoAr(eCO2, TVOC, c_mgm3_PM25, c_mgm3_PM1);
    String qualidade_str;
    if (qualidadeDoAr == 0)
      qualidade_str = "Ruim";
    else if (qualidadeDoAr == 1)
      qualidade_str = "Regular";
    else
      qualidade_str = "Bom";

    JsonDocument dados;
    dados["codigo"] = String(num_dispositivo);
    dados["sala"] = sala;
    dados["temperatura"] = temperatura;
    dados["qualidade"] = qualidade_str;

    String dados_out;
    serializeJson(dados, dados_out);

    mqtt.publish("arcondicionado/enviaDados", dados_out);
    Serial.println(dados_out);

    desenhaEsqueleto();
    desenhaDados(temperatura, umidade, qualidadeDoAr);

    apitaBuzzer();
    instanteAnterior = instanteAtual;
  }
}
