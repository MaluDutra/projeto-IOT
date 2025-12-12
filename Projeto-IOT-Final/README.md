# Projeto Totem de Ar Condicionado Inteligente

## 📋 Descrição do Projeto

O **Totem de Ar Condicionado Inteligente** é um sistema de automação desenvolvido para ambientes acadêmicos que permite o controle democratizado de aparelhos de ar condicionado através de votações via bot do Telegram. O sistema monitora continuamente a qualidade do ar, fornecendo dados essenciais para manutenções preventivas e garantindo um ambiente confortável e saudável.

### Principais Funcionalidades

- **Controle por Votação**: Sistema democrático de controle via bot do Telegram
- **Monitoramento de Qualidade do Ar**: Sensores que coletam dados em tempo real
- **Gestão de Alunos**: Servidor web para cadastro e verificação de alunos, aulas, aparelhos, etc.
- **Visualização de Dados**: Dashboard interativo no Grafana para análise de métricas

### Potencial de Expansão

Embora desenvolvido inicialmente para o ambiente acadêmico, o projeto pode ser facilmente adaptado para:
- Escritórios corporativos
- Espaços de coworking
- Bibliotecas públicas
- Centros comunitários
- Qualquer ambiente compartilhado que necessite controle democrático de climatização

---

## 🏗️ Arquitetura do Sistema

O projeto utiliza uma arquitetura distribuída baseada em comunicação MQTT, processamento de dados com Node-RED e visualização através do Grafana.

### Componentes Principais

```
┌─────────────────┐      MQTT     ┌──────────────┐
│ ESP32 + Sensores│  ───────────► │   Node-RED   │
│  (Hardware)     │               │(Orquestrador)│
└─────────────────┘               └──────┬───────┘
                                         │
                    ┌────────────────────│
                    │                    │                    
              ┌─────▼─────┐       ┌─────▼─────┐        ┌─────▼─────┐
              │  Telegram │       │  Database │───────►│  Grafana  │                               
              │    Bot    │       │   (SQL)   │        │ Dashboard │
              └───────────┘       └───────────┘        └───────────┘
                                         ▲
                                         │
                                  ┌──────▼──────┐
                                  │   Servidor  │
                                  │   Flask     │
                                  │ (Cadastros) │
                                  └─────────────┘
```

### Fluxo de Dados

1. **Coleta de Dados**: O ESP32 equipado com sensores coleta dados de temperatura, umidade e qualidade do ar
2. **Comunicação MQTT**: Os dados são enviados via protocolo MQTT para o broker
3. **Processamento Node-RED**: Node-RED recebe os dados, processa regras de negócio e gerencia votações
4. **Armazenamento**: Dados são persistidos em banco de dados PostgreSQL
5. **Visualização**: Grafana consome os dados para exibição em dashboards
6. **Interação**: Usuários votam através do bot do Telegram
7. **Gestão**: Servidor Flask gerencia cadastros de alunos, salas, turmas e horários

### Tecnologias Utilizadas

- **Hardware**: ESP32, Sensores de qualidade do ar, Display Epaper, Sensor infravermelho e Placa de cobre para o circuito
- **Protocolo**: MQTT (Message Queuing Telemetry Transport)
- **Orquestração**: Node-RED
- **Backend**: Python Flask
- **Banco de Dados**: PostgreSQL
- **Visualização**: Grafana
- **Interface de Usuário**: Telegram Bot
- **Firmware**: PlatformIO (C++)

---

## 📡 MQTT (Message Queuing Telemetry Transport)

O protocolo MQTT é o núcleo da comunicação entre o hardware (ESP32) e o sistema de processamento. Ele foi escolhido por ser leve, eficiente e ideal para dispositivos IoT.

### Estrutura de Tópicos

O sistema utiliza uma hierarquia organizada de tópicos MQTT para gerenciar a comunicação:

# Gerado pela IA, sei lá se ta certo
```
totem/
├── sensores/
│   ├── temperatura        # Dados de temperatura em °C
│   ├── umidade           # Dados de umidade relativa (%)
│   ├── qualidade_ar      # Índice de qualidade do ar (PPM)
│   └── status            # Status geral dos sensores
├── controle/
│   ├── comando           # Comandos para ligar/desligar AC
│   ├── temperatura_alvo  # Temperatura desejada
│   └── modo              # Modo de operação (auto/manual)
├── votacao/
│   ├── inicio            # Início de nova votação
│   ├── votos             # Contabilização de votos
│   └── resultado         # Resultado da votação
└── display/
    ├── atualizar         # Atualização do display e-paper
    └── mensagem          # Mensagens para exibição
```

### Broker MQTT

# Conferir tbm pfvr
- **Software**: Mosquitto MQTT Broker
- **Porta**: 1883 (padrão) ou 8883 (TLS/SSL)
- **Segurança**: Autenticação por usuário/senha e suporte a certificados TLS

### Exemplo de Payload

```json
{
  "timestamp": "2025-12-11T14:30:00Z",
  "temperatura": 24.5,
  "sala": "LET",
  "qualidade": "Bom",
  "codigo": "1"
}
```

### Configuração no ESP32

O ESP32 se conecta ao broker MQTT e publica dados dos sensores a cada 30 segundos, além de subscrever aos tópicos de controle para receber comandos em tempo real.

---

## 🔀 Node-RED

O Node-RED atua como o orquestrador central do sistema, processando dados, gerenciando lógica de negócio e coordenando a comunicação entre todos os componentes.

### Fluxos Principais

![Node-RED Flows](docs/images/nodered-flows.png)

*Visualização dos fluxos principais do Node-RED.*

#### 1. Fluxo de Processamento de Sensores

```
[MQTT In] → [Validação] → [Transformação] → [Database] → [Grafana]
                                         ↓
                                   [Alertas]
```

- Recebe dados dos sensores via MQTT
- Valida e formata os dados
- Armazena no banco de dados PostgreSQL
- Envia para o Grafana
- Gera alertas quando valores críticos são detectados

#### 2. Fluxo de Votação do Telegram

```
[Telegram Bot] → [Validação Usuário] → [Contabilização] → [Decisão] → [MQTT Out]
                        ↓                                               ↓
                   [Database]                                    [Ativar/Desativar AC]
```

- Recebe votos dos usuários via Telegram
- Valida se o usuário está cadastrado e tem aula no momento
- Contabiliza os votos (maioria simples)
- Toma decisão e envia comando via MQTT
- Registra a votação no banco de dados

#### 3. Fluxo de Agendamento

```
[Cron/Scheduler] → [Consulta DB] → [Verificar Horário] → [Ação Automática]
                                           ↓
                                    [Telegram Notify]
```

- Verifica automaticamente os horários de aula
- Envia notificações aos alunos no início das aulas
- Ativa/desativa o sistema conforme agenda
- Realiza manutenções preventivas programadas

### Funcionalidades Implementadas

- **Integração MQTT**: Nós de entrada/saída para comunicação com ESP32
- **Bot Telegram**: Gerenciamento completo de comandos e votações
- **Queries SQL**: Consultas ao PostgreSQL para validação e armazenamento
- **Lógica de Votação**: Algoritmo de contagem e decisão democrática
- **Notificações**: Alertas automáticos para usuários e administradores
- **Tratamento de Erros**: Logs e recuperação de falhas

### Exemplo de Função JavaScript

```javascript
// Processamento de votação
let votosLigar = flow.get('votos_ligar') || 0;
let votosDesligar = flow.get('votos_desligar') || 0;

if (msg.payload.voto === 'ligar') {
    votosLigar++;
    flow.set('votos_ligar', votosLigar);
} else {
    votosDesligar++;
    flow.set('votos_desligar', votosDesligar);
}

if (votosLigar + votosDesligar >= msg.payload.totalAlunos / 2) {
    msg.payload = {
        comando: votosLigar > votosDesligar ? 'ON' : 'OFF',
        votos_ligar: votosLigar,
        votos_desligar: votosDesligar
    };
    flow.set('votos_ligar', 0);
    flow.set('votos_desligar', 0);
    return msg;
}
```

### Importação dos Fluxos

Os fluxos completos estão disponíveis em `nodered-flows/flows.json`. Para importá-los:

1. Acesse o Node-RED (geralmente em `http://localhost:1880`)
2. Menu → Import → Clipboard
3. Cole o conteúdo do arquivo `flows.json`
4. Configure as credenciais (Telegram Token, MQTT, Database)

---

## 🗄️ Banco de Dados (PostgreSQL)

O PostgreSQL é utilizado para persistir todos os dados do sistema, desde cadastros até histórico de sensores e votações.

### Diagrama Entidade-Relacionamento

![Diagrama ER](/images/database-er.png)

*Modelo de dados do sistema.*

### Estrutura das Tabelas Principais

#### 1. Tabela: `alunos`
```sql
CREATE TABLE alunos (
    matricula VARCHAR(8) PRIMARY KEY,
    nome VARCHAR(255) NOT NULL,
    data_nascimento DATE,
    genero VARCHAR(255),
    id_chat VARCHAR(255)  -- ID do chat do Telegram
);
```
*Armazena informações dos alunos cadastrados no sistema, incluindo seus identificadores do Telegram para votações.*

#### 2. Tabela: `salas`
```sql
CREATE TABLE salas (
    codigo VARCHAR(8) PRIMARY KEY,
    local VARCHAR(255) NOT NULL
);
```
*Cadastro das salas/ambientes onde os aparelhos estão instalados.*

#### 3. Tabela: `aparelhos`
```sql
CREATE TABLE aparelhos (
    codigo INT,
    sala VARCHAR(8) REFERENCES salas(codigo),
    qualidade VARCHAR(255), -- Ex: 'Bom', 'Ruim'
    PRIMARY KEY (codigo, sala)
);
```
*Registro dos dispositivos/aparelhos de ar condicionado disponíveis no sistema.*

#### 4. Tabela: `aulas`
```sql
CREATE TABLE aulas (
    codigo VARCHAR(8) PRIMARY KEY,
    nome VARCHAR(255)
);
```
*Registro das aulas no sistema.*

#### 5. Tabela: `aulas_horas`
```sql
CREATE TABLE aulas_horas (
    aula VARCHAR(8) REFERENCES aulas(codigo),
    turma VARCHAR(8),
    dia VARCHAR(8),
    sala VARCHAR(8) REFERENCES salas(codigo)
    PRIMARY KEY (sala, turma, dia)
);
```
*Define as aulas programadas, associando salas, turmas e dias da semana*

#### 6. Tabela: `alunos_aulas`
```sql
CREATE TABLE alunos_aulas (
    matricula VARCHAR(8) REFERENCES alunos(matricula),
    aula VARCHAR(8) REFERENCES aulas_horas(aula),
    turma VARCHAR(8) REFERENCES aulas_horas(turma),
    PRIMARY KEY (matricula, aula, turma)
);
```
*Relaciona alunos às suas aulas, permitindo validação de participação em votações.*

#### 7. Tabela: `salas_temperatura`
```sql
CREATE TABLE salas_temperatura (
    sala VARCHAR(8) REFERENCES salas(codigo),
    temperatura FLOAT NOT NULL,
    data_hora TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
    PRIMARY KEY (sala, data_hora)
);
```
*Armazena histórico de leituras de temperatura por sala, coletadas pelos sensores dos aparelhos.*

### Conexão com o Sistema

- **Servidor Flask**: Utiliza psycopg para ORM
- **Node-RED**: Conexão direta via nó PostgreSQL
- **Grafana**: Data source PostgreSQL para dashboards

---

## 🔌 Esquemático do Circuito

### Diagrama Esquemático

![Esquemático do Circuito](docs/images/schematic.png)

*Esquemático desenvolvido no EasyEDA mostrando as conexões do ESP32 com os sensores e módulos.*

### Componentes Eletrônicos

- **Microcontrolador**: ESP32
- **Sensores**: 
  - Sensor de temperatura e umidade (DHT22/BME280)
  - Sensor de qualidade do ar (MQ-135/CCS811)
  - Sensor infravermelhor para controlar o aparelho
- **Comunicação**: Módulo Wi-Fi integrado no ESP32
- **Alimentação**: Regulador de tensão 5V/3.3V
- **Interface**: Display Epaper

---

## 📟 Placa de Circuito Impresso (PCB)

### Vista Frontal da Placa

![PCB - Frente](docs/images/pcb-front.png)

*Camada superior da placa com componentes SMD e through-hole.*

### Vista Posterior da Placa

![PCB - Verso](docs/images/pcb-back.png)

*Camada inferior mostrando as trilhas e plano de terra.*

---

## 📊 Dashboards e Visualizações

### Dashboard Principal - Monitoramento em Tempo Real

![Dashboard Grafana - Overview](docs/images/dashboard-overview.png)

*Visão geral com métricas de temperatura e qualidade do ar em tempo real.*

### Dashboard de Manutenção

![Dashboard Grafana - Manutenção](images/dashboard-maintenance.png)

*Indicadores de qualidade do ar e alertas para manutenção preventiva.*

### Interface do Servidor de Cadastros

![Servidor Flask - Cadastros](images/server-interface.png)

*Interface web para gerenciamento de alunos, turmas, salas e horários.*

### Formulários de Cadastro e Edição

![Servidor Flask - Formulários1](images/server-forms1.png)
![Servidor Flask - Formulários2](images/server-forms2.png)

*Interface web para cadastro e edição.*

### Página de Listagem de Itens

![Servidor Flask - Listagem](imagens/server-list.png)

*Interface web para listar todos os itens de uma categoria.*

---

## 🎥 Demonstração em Vídeo

<div align="center">
  <iframe width="800" height="450" 
    src="https://www.youtube.com/embed/VIDEO_ID_AQUI" 
    title="Demonstração do Projeto Totem de Ar Condicionado Inteligente" 
    frameborder="0" 
    allow="accelerometer; autoplay; clipboard-write; encrypted-media; gyroscope; picture-in-picture" 
    allowfullscreen>
  </iframe>
</div>

*Vídeo demonstrando o funcionamento completo do sistema, desde a votação no Telegram até a ativação do ar condicionado e visualização no Grafana.*

---

## 🚀 Como Executar o Projeto

### Pré-requisitos

- PlatformIO (para firmware do ESP32)
- Python 3.8+
- Node-RED
- Grafana
- PostgreSQL
- Broker MQTT (Mosquitto)

### Configuração do Hardware

1. Grave o firmware no ESP32 usando PlatformIO
2. Configure as credenciais Wi-Fi e certificados em `include/certificados.h`
3. Conecte os sensores conforme o esquemático

### Configuração do Software

1. **Banco de Dados**: Execute os scripts de criação do banco
2. **Node-RED**: Importe o flow de `nodered-flows/flows.json`
3. **Servidor Flask**: 
   ```bash
   cd python
   pip install -r requirements.txt # psycopg necessita de um compilador C instalado (funcionou no linux e no windos do LET)
   python main.py
   ```
4. **Grafana**: Configure as dashboards e data sources
5. **Telegram Bot**: Configure o token do bot no Node-RED

---

## 👥 Equipe de Desenvolvimento

*[Adicione aqui informações sobre a equipe]*

---

## 🔄 Atualizações Futuras

- [ ] Integração com assistentes de voz (Alexa, Google Assistant)
- [ ] Aplicativo mobile dedicado
- [ ] Suporte a múltiplas salas simultaneamente
- [ ] Machine Learning para predição de padrões de uso
- [ ] API REST para integrações externas

---

**Desenvolvido com ❤️ para tornar ambientes compartilhados mais confortáveis e eficientes.**
