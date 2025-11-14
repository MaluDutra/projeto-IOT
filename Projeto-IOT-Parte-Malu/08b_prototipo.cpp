#include <Arduino.h>
#include <WiFi.h>
#include <WiFiClientSecure.h>
#include "certificados.h"
#include <MQTT.h>
#include <ArduinoJson.h>

WiFiClientSecure conexaoSegura;
MQTTClient mqtt(1000);

JsonDocument listaChatIds; // aqueles ainda não cadastrados
JsonDocument listaAlunos;
// lista de Jsons de alunos:
// matricula do aluno
// id do chat
// se está alterando o dispositivo --> começa com false
// se está em votação --> começa com false

String procurarChatId(String chatId)
{
  Serial.print("\nChat Id a ser procurado: ");
  Serial.println(chatId);

  for (unsigned int i = 0; i < listaChatIds.size(); i++)
  {
    if (chatId == listaChatIds[i])
    {
      Serial.println("Encontrou!\n");
      return listaChatIds[i];
    }
    Serial.println(String(listaChatIds[i]));
  }

  Serial.println("Não encontrado!\n");
  return "";
}

int procurarAlunoMatricula(String matricula)
{
  Serial.print("\nMatrícula a ser procurada: ");
  Serial.println(matricula);

  for (unsigned int i = 0; i < listaAlunos.size(); i++)
  {
    JsonDocument aux = listaAlunos[i];

    if (matricula == aux[0])
    {
      Serial.println("Encontrou!\n");
      return i;
    }
    Serial.println(String(aux[0]));
  }

  Serial.println("Não encontrado!\n");
  return -1;
}

int procurarAlunoChatId(String chatId)
{
  Serial.print("\nChat Id a ser procurado: ");
  Serial.println(chatId);

  for (unsigned int i = 0; i < listaAlunos.size(); i++)
  {
    JsonDocument aux = listaAlunos[i];

    if (chatId == aux[1])
    {
      Serial.println("Encontrou!\n");
      return i;
    }
    Serial.println(String(aux[1]));
  }

  Serial.println("Não encontrado!\n");
  return -1;
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
      mqtt.connect("8883", "aula", "zowmad-tavQez"); // verificar!!
      Serial.print(".");
      delay(1000);
    }
    Serial.println(" conectado!");

    mqtt.subscribe("arcondicionado/telegram", 1); // verificar!!
  }
}

// inicializar lista alunos --> percorrer o banco de dados, verificar aqueles que já tem chatId e adicioná-los à lista
void inicializarListaAlunos()
{

}

void recebeuMensagem(String topico, String conteudo)
{
  Serial.println(topico + ": " + conteudo);

  if (topico.startsWith("arcondicionado/telegram")) // verificar!!
  {
    // dentro de conteúdo, preciso ter a mensagem e id do chat --> JSON
    JsonDocument mensagem;
    deserializeJson(mensagem, conteudo);

    String chatId = mensagem["chatId"];
    String conteudo_mensagem = mensagem["content"];
    Serial.print("chatId: ");
    Serial.println(chatId);
    Serial.print("content: ");
    Serial.println(conteudo_mensagem);

    if (conteudo_mensagem == "Iniciar Votação")
    {
      // verificar se já não há uma votação em andamento
      // se já houver, pedir para aguardar o resultado
      // senão:
      // enviar mensagem para todos os alunos da sala do determinado aluno que iniciou a votação
      // esperar a resposta deles por um tempo determinado
      // verificar o resultado
      // se a mudança ganhou, alterar a temperatura do ar enviando um sinal infravermelho
      // senão, não fazer nada
    }
    else if (conteudo_mensagem == "Temperatura Atual")
    {
      // pegar qual é a sala atual em que o aluno está
      // encontrar a temperatura da sala atual no banco de dados
      // enviar mensagem dizendo a temperatura --> publish
    }
    else if (conteudo_mensagem == "Alterar Dispositivo")
    {
      // alterando_dispositivo = true;
      // perguntar a matrícula
      // remover o chat id relacionado ao aluno no banco de dados
      // informar que ele precisará cadastrar novamente no novo dispositivo
    }
    else if (conteudo_mensagem.startsWith("Matrícula:"))
    {
      // if (alterando_dispositivo)
      // {
      //   // pegar a matrícula, acessar o banco de dados e remover o chat id dessa chave
      //   alterando_dispositivo = false;
      // }
      // else if (cadastrando_dispositivo)
      // {
      //   // pegar a matrícula, acessar o banco de dados e adicionar o chat id nessa chave
      //   cadastrando_dispositivo = false;
      // }
    }
    else if (conteudo_mensagem == "Sim")
    {
    }
    else if (conteudo_mensagem == "Não")
    {
    }
    else
    {
      // verificar se o id da conversa já está cadastrado no banco de dados
      if (procurarAlunoChatId(chatId) == -1)
      {
        // se não estiver cadastrado, perguntar a matrícula do aluno e cadastrar o id no banco
        // cadastrando_dispositivo = true;
      }
      else
      {
        // se já tiver cadastrado, mostrar as opções de conversa
      }
    }
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
  mqtt.setWill("arcondicionado/dispositivo", "Dispositivo desconectado");
  reconectarMQTT();

  inicializarListaAlunos();
}

void loop()
{
  reconectarWiFi();
  reconectarMQTT();
  mqtt.loop();
}
