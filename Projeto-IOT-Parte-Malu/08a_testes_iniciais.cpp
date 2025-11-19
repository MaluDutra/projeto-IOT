#include <Arduino.h>
#include <WiFi.h>
#include <WiFiClientSecure.h>
#include "certificados.h"
#include <MQTT.h>
#include <ArduinoJson.h>
#include <IRremoteESP8266.h>
#include <IRac.h>
#include <IRutils.h>
#include <string>

WiFiClientSecure conexaoSegura;
MQTTClient mqtt(1000);

const uint16_t IRled = 4;    // pino do bagulho que envia
IRac ar_condicionado(IRled); // classe que realiza o controle genérico do ar condicionado

int num_dispositivo = 1;

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

void reconectarWiFi()
{
  if (WiFi.status() != WL_CONNECTED)
  {
    WiFi.begin("Projeto", "2022-11-07"); // verificar!!
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
      mqtt.connect("8883", "aula", "zowmad-tavQez");!
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

void setup()
{
  Serial.begin(115200);
  delay(500);
  Serial.println("Projeto - Telegram");

  reconectarWiFi();
  conexaoSegura.setCACert(certificado1);

  mqtt.begin("mqtt.janks.dev.br", 8883, conexaoSegura);
  mqtt.onMessage(recebeuMensagem);
  mqtt.setKeepAlive(10);
  
  char topico_will[] = "arcondicionado/dispositivos/desconexao";
  char mensagem_will[] = "%s", String(num_dispositivo);
  mqtt.setWill(topico_will, mensagem_will);
  reconectarMQTT();

  setup_AC();
  Serial.println("Estado inicial do ar:");
  imprimeEstadoDoAr(estadoDoAr);
  Serial.println("==============================================\n");
}

unsigned int instanteAnterior = 0;
bool ligado = false;
void loop()
{
  reconectarWiFi();
  reconectarMQTT();
  mqtt.loop();

  unsigned long instanteAtual = millis();
  if (instanteAtual > instanteAnterior + 5000)
  {
    Serial.println("+20 segundo");

    estadoDoAr = ar_condicionado.getState(); // Para pegar o estado do ar
    imprimeEstadoDoAr(estadoDoAr);

    ACControle comandoDoAr;
    // comandoDoAr.tipo = comando::Temperature;
    // float minTemp = 17.0f;
    // float maxTemp = 25.0f;
    // comandoDoAr.instrucao.temperatura = minTemp + static_cast<float>(rand()) / (static_cast<float>(RAND_MAX / (maxTemp - minTemp)));
    comandoDoAr.tipo = comando::Power;
    // comandoDoAr.instrucao.ligar = false;
    // envia_comando(&comandoDoAr);

    if (ligado)
    {
      Serial.println("Vai desligar!!");
      comandoDoAr.instrucao.ligar = false;
      ligado = false;
    }
    else
    {
      Serial.println("Vai ligar!!");
      comandoDoAr.instrucao.ligar = true;
      ligado = true;
    }
    envia_comando(&comandoDoAr);

    instanteAnterior = instanteAtual;
  }
}