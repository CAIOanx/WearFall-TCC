#include <Arduino.h>
#include <Wire.h>
#include <WiFi.h>
#include <WebServer.h>
#include <Preferences.h>
#include <math.h>
#include "dashboard_assets.h"

const bool WIFI_ATIVADO = true;
const char* AP_SSID = "WearFall_AP";
const char* AP_SENHA = "wearfallof"; // minimo 8 caracteres
const int PIN_BUZZER = 18;
const int PIN_LED = 4;
const int PIN_BOTAO = 19;
const uint8_t IMU_ENDERECO = 0x68;
const uint8_t REG_SMPLRT_DIV   = 0x19;
const uint8_t REG_CONFIG       = 0x1A;
const uint8_t REG_GYRO_CONFIG  = 0x1B;
const uint8_t REG_ACCEL_CONFIG = 0x1C;
const uint8_t REG_ACCEL_XOUT_H = 0x3B;
const uint8_t REG_PWR_MGMT_1   = 0x6B;
const uint8_t REG_WHO_AM_I     = 0x75;


const float ESCALA_ACEL = 4096.0;
const float ESCALA_GIRO = 65.5;
const int INTERVALO_AMOSTRAGEM = 20;
const float LIMIAR_QUEDA_LIVRE = 0.6;
const float LIMIAR_IMPACTO = 2.5;
const unsigned long JANELA_IMPACTO = 3000;
const unsigned long JANELA_POS_IMPACTO = 6000;
const unsigned long ATRASO_ANALISE_POSTURA = 500;
const float LIMIAR_ORIENTACAO_Z = 0.5;
const unsigned long TEMPO_POSTURA_ALTERADA = 800;
const float TOLERANCIA_IMOBILIDADE = 0.15;
const unsigned long TEMPO_IMOBILIDADE = 800;
const float LIMIAR_ROTACAO = 150.0;
const int TAMANHO_HISTORICO = 10;
enum EstadoSistema {
  MONITORANDO,
  AGUARDANDO_IMPACTO,
  ANALISE_POS_IMPACTO,
  ALERTA
};
volatile EstadoSistema estadoSistema = MONITORANDO;
struct LeituraIMU {
  float ax;
  float ay;
  float az;
  float gx;
  float gy;
  float gz;
  unsigned long tempo;
};
struct RegistroEvento {
  unsigned long numero;
  unsigned long tempo;
  bool rotacao;
  bool posturaAlterada;
  bool imobilidade;
  bool quedaConfirmada;
};
WebServer servidor(80);
bool servidorAtivo = false;
Preferences preferences;
bool persistenciaAtiva = false;
QueueHandle_t filaIMU;
SemaphoreHandle_t mutexHistorico;
RegistroEvento historico[TAMANHO_HISTORICO];
int indiceHistorico = 0;
int totalHistorico = 0;
unsigned long contadorEventos = 0;
const char* NVS_NAMESPACE = "wf_hist";
const uint32_t NVS_VERSAO = 1;
bool escreverRegistroIMU(uint8_t registro, uint8_t valor);
bool lerRegistradoresIMU(uint8_t registroInicial, uint8_t* buffer, size_t quantidade);
bool inicializarIMU();
bool lerIMU(LeituraIMU &dado);
void taskAmostragem(void *pvParameters);
void taskProcessamento(void *pvParameters);
void taskControle(void *pvParameters);
const char* nomeEstado(EstadoSistema estado);
void registrarEvento(
  bool rotacao,
  bool postura,
  bool imobilidade,
  bool confirmada
);
void mostrarHistorico();
void limparHistorico();
bool inicializarPersistencia();
bool carregarHistoricoPersistente();
bool salvarHistoricoPersistente();
void fecharPersistencia();
bool conectarWiFi();
void paginaInicial();
void paginaEstilo();
void paginaScript();
void apiStatus();
void apiHistorico();
void apiCancelar();
void apiOpcoes();
void cancelarAlerta(const char* origem);
void enviarJson(int codigo, const String &json);
void setup() {
  Serial.begin(115200);
  delay(1000);
  Serial.println();
  Serial.println("==============================================");
  Serial.println("WEARFALL 2.3 - FINAL LOCAL");
  Serial.println("I2C + DETECCAO + HISTORICO PERSISTENTE + AP + WEB");
  Serial.println("==============================================");
  Serial.println();
  pinMode(PIN_LED, OUTPUT);
  pinMode(PIN_BUZZER, OUTPUT);
  pinMode(PIN_BOTAO, INPUT_PULLUP);
  digitalWrite(PIN_LED, LOW);
  noTone(PIN_BUZZER);
  Wire.begin(21, 22);
  Wire.setClock(400000);
  if (!inicializarIMU()) {
    Serial.println("ERRO: IMU nao inicializada!");
    while (true) {
      delay(500);
    }
  }
  Serial.println("IMU inicializada por leitura direta I2C.");
  filaIMU = xQueueCreate(
    20,
    sizeof(LeituraIMU)
  );
  if (filaIMU == NULL) {
    Serial.println(
      "ERRO: Falha ao criar fila!"
    );
    while (true) {
      delay(500);
    }
  }
  mutexHistorico =
    xSemaphoreCreateMutex();
  if (mutexHistorico == NULL) {
    Serial.println(
      "ERRO: Falha ao criar mutex!"
    );
    while (true) {
      delay(500);
    }
  }
  Serial.println(
    "Estrutura do historico inicializada."
  );
  if (inicializarPersistencia()) {
    carregarHistoricoPersistente();
  } else {
    Serial.println(
      "AVISO: persistencia NVS indisponivel. Historico funcionara apenas em RAM."
    );
  }
  if (WIFI_ATIVADO && conectarWiFi()) {
    servidor.on("/", HTTP_GET, paginaInicial);
    servidor.on("/index.html", HTTP_GET, paginaInicial);
    servidor.on("/style.css", HTTP_GET, paginaEstilo);
    servidor.on("/script.js", HTTP_GET, paginaScript);
    servidor.on("/api/status", HTTP_GET, apiStatus);
    servidor.on("/api/history", HTTP_GET, apiHistorico);
    servidor.on("/api/cancel", HTTP_POST, apiCancelar);
    servidor.on("/api/status", HTTP_OPTIONS, apiOpcoes);
    servidor.on("/api/history", HTTP_OPTIONS, apiOpcoes);
    servidor.on("/api/cancel", HTTP_OPTIONS, apiOpcoes);
    servidor.onNotFound([]() { servidor.send(404, "text/plain", "Rota nao encontrada"); });
    servidor.begin();
    servidorAtivo = true;
    Serial.println("Servidor HTTP e dashboard iniciados!");
    Serial.print("Acesse: http://");
    Serial.println(WiFi.softAPIP());
  } else {
    Serial.println("Dashboard offline: Wi-Fi desativado ou AP indisponivel.");
  }
  Serial.println();
  Serial.println("==================================");
  Serial.println("WEARFALL - MODO LOCAL");
  Serial.println("Monitoramento local, sem internet.");
  Serial.println("Dashboard: http://192.168.4.1");
  Serial.println("==================================");
  Serial.println();
  xTaskCreatePinnedToCore(
    taskAmostragem,
    "Amostragem",
    3000,
    NULL,
    2,
    NULL,
    0
  );
  xTaskCreatePinnedToCore(
    taskProcessamento,
    "Processamento",
    4500,
    NULL,
    1,
    NULL,
    1
  );
  xTaskCreatePinnedToCore(
    taskControle,
    "Controle",
    3000,
    NULL,
    1,
    NULL,
    1
  );
  Serial.println();
  Serial.println(
    "Sistema iniciado."
  );
  Serial.println(
    "H = mostrar historico"
  );
  Serial.println(
    "L = limpar historico"
  );
  Serial.println();
  Serial.println(
    "Estado inicial: MONITORANDO"
  );
  Serial.println();
}
void loop() {
  
  if (servidorAtivo) {
    servidor.handleClient();
  }
  
  if (Serial.available() > 0) {
    char comando =
      Serial.read();
    if (
      comando == 'H' ||
      comando == 'h'
    ) {
      mostrarHistorico();
    }
    else if (
      comando == 'L' ||
      comando == 'l'
    ) {
      limparHistorico();
    }
  }
  delay(2);
}
bool conectarWiFi() {
  Serial.println("Iniciando rede local WearFall_AP...");
  WiFi.mode(WIFI_AP);
  if (!WiFi.softAP(AP_SSID, AP_SENHA)) {
    Serial.println("ERRO: falha ao criar WearFall_AP.");
    return false;
  }
  Serial.print("Dashboard local em http://");
  Serial.println(WiFi.softAPIP());
  return true;
}

void paginaInicial() {
  servidor.sendHeader("Cache-Control", "no-store");
  servidor.send_P(200, "text/html; charset=utf-8", DASHBOARD_HTML);
}
void paginaEstilo() {
  servidor.send_P(200, "text/css; charset=utf-8", DASHBOARD_CSS);
}
void paginaScript() {
  servidor.send_P(200, "application/javascript; charset=utf-8", DASHBOARD_JS);
}
void enviarJson(int codigo, const String &json) {
  servidor.sendHeader("Access-Control-Allow-Origin", "*");
  servidor.sendHeader("Access-Control-Allow-Methods", "GET, POST, OPTIONS");
  servidor.sendHeader("Access-Control-Allow-Headers", "Content-Type");
  servidor.sendHeader("Cache-Control", "no-store");
  servidor.send(codigo, "application/json; charset=utf-8", json);
}
void apiOpcoes() {
  servidor.sendHeader("Access-Control-Allow-Origin", "*");
  servidor.sendHeader("Access-Control-Allow-Methods", "GET, POST, OPTIONS");
  servidor.sendHeader("Access-Control-Allow-Headers", "Content-Type");
  servidor.send(204);
}
void apiStatus() {
  EstadoSistema estado = estadoSistema;
  String resposta = "{\"estado\":\"";
  resposta += nomeEstado(estado);
  resposta += "\",\"alerta\":";
  resposta += (estado == ALERTA ? "true" : "false");
  
  resposta += ",\"bateria\":null}";
  enviarJson(200, resposta);
}
void apiHistorico() {
  RegistroEvento copia[TAMANHO_HISTORICO];
  int quantidade = 0;
  if (xSemaphoreTake(mutexHistorico, pdMS_TO_TICKS(150)) != pdTRUE) {
    enviarJson(503, "{\"erro\":\"historico_ocupado\"}");
    return;
  }
  quantidade = totalHistorico;
  
  for (int i = 0; i < quantidade; ++i) {
    int indice = (indiceHistorico - 1 - i + TAMANHO_HISTORICO) % TAMANHO_HISTORICO;
    copia[i] = historico[indice];
  }
  xSemaphoreGive(mutexHistorico);
  String json;
  json.reserve(quantidade * 125 + 20);
  json = "{\"eventos\":[";
  for (int i = 0; i < quantidade; ++i) {
    if (i) json += ',';
    json += "{\"numero\":"; json += String(copia[i].numero);
    json += ",\"tempoMs\":"; json += String(copia[i].tempo);
    json += ",\"rotacao\":"; json += (copia[i].rotacao ? "true" : "false");
    json += ",\"postura\":"; json += (copia[i].posturaAlterada ? "true" : "false");
    json += ",\"imobilidade\":"; json += (copia[i].imobilidade ? "true" : "false");
    json += ",\"quedaConfirmada\":"; json += (copia[i].quedaConfirmada ? "true" : "false");
    json += '}';
  }
  json += "]}";
  enviarJson(200, json);
}
void cancelarAlerta(const char* origem) {
  if (estadoSistema != ALERTA) return;
  estadoSistema = MONITORANDO;
  Serial.print("ALERTA cancelado por: ");
  Serial.println(origem);
}
void apiCancelar() {
  if (estadoSistema != ALERTA) {
    enviarJson(409, "{\"ok\":false,\"motivo\":\"sem_alerta\"}");
    return;
  }
  cancelarAlerta("dashboard");
  enviarJson(200, "{\"ok\":true}");
}
const char* nomeEstado(
  EstadoSistema estado
) {
  switch (estado) {
    case MONITORANDO:
      return "MONITORANDO";
    case AGUARDANDO_IMPACTO:
      return "AGUARDANDO IMPACTO";
    case ANALISE_POS_IMPACTO:
      return "ANALISE POS-IMPACTO";
    case ALERTA:
      return "ALERTA";
    default:
      return "DESCONHECIDO";
  }
}
void registrarEvento(
  bool rotacao,
  bool postura,
  bool imobilidade,
  bool confirmada
) {
  if (
    xSemaphoreTake(
      mutexHistorico,
      pdMS_TO_TICKS(100)
    ) == pdTRUE
  ) {
    contadorEventos++;
    RegistroEvento novoEvento;
    novoEvento.numero =
      contadorEventos;
    novoEvento.tempo =
      millis();
    novoEvento.rotacao =
      rotacao;
    novoEvento.posturaAlterada =
      postura;
    novoEvento.imobilidade =
      imobilidade;
    novoEvento.quedaConfirmada =
      confirmada;
    historico[
      indiceHistorico
    ] = novoEvento;
    indiceHistorico =
      (
        indiceHistorico + 1
      ) %
      TAMANHO_HISTORICO;
    if (
      totalHistorico <
      TAMANHO_HISTORICO
    ) {
      totalHistorico++;
    }
    if (persistenciaAtiva) salvarHistoricoPersistente();
    xSemaphoreGive(
      mutexHistorico
    );
    Serial.println();
    Serial.print(
      "Evento #"
    );
    Serial.print(
      novoEvento.numero
    );
    Serial.println(
      " registrado no historico."
    );
    Serial.println();
  }
}
void mostrarHistorico() {
  if (
    xSemaphoreTake(
      mutexHistorico,
      pdMS_TO_TICKS(100)
    ) != pdTRUE
  ) {
    return;
  }
  Serial.println();
  Serial.println(
    "=============================================="
  );
  Serial.println(
    "HISTORICO DE EVENTOS - WEARFALL"
  );
  Serial.println(
    "=============================================="
  );
  if (
    totalHistorico == 0
  ) {
    Serial.println(
      "Nenhum evento registrado."
    );
    Serial.println();
    xSemaphoreGive(
      mutexHistorico
    );
    return;
  }
  int inicio;
  if (
    totalHistorico <
    TAMANHO_HISTORICO
  ) {
    inicio = 0;
  }
  else {
    inicio =
      indiceHistorico;
  }
  for (
    int i = 0;
    i < totalHistorico;
    i++
  ) {
    int indice =
      (
        inicio + i
      ) %
      TAMANHO_HISTORICO;
    RegistroEvento evento =
      historico[indice];
    Serial.println();
    Serial.print("Evento #");
    Serial.println(evento.numero);
    Serial.print(
      "Tempo: "
    );
    Serial.print(
      evento.tempo
    );
    Serial.println(
      " ms"
    );
    Serial.print(
      "Rotacao brusca: "
    );
    Serial.println(
      evento.rotacao
      ? "SIM"
      : "NAO"
    );
    Serial.print(
      "Postura alterada: "
    );
    Serial.println(
      evento.posturaAlterada
      ? "SIM"
      : "NAO"
    );
    Serial.print(
      "Imobilidade: "
    );
    Serial.println(
      evento.imobilidade
      ? "SIM"
      : "NAO"
    );
    Serial.print(
      "Resultado: "
    );
    Serial.println(
      evento.quedaConfirmada
      ? "QUEDA CONFIRMADA"
      : "EVENTO DESCARTADO"
    );
    Serial.println(
      "----------------------------------------------"
    );
  }
  Serial.print(
    "Eventos armazenados: "
  );
  Serial.print(
    totalHistorico
  );
  Serial.print("/");
  Serial.println(
    TAMANHO_HISTORICO
  );
  Serial.println();
  xSemaphoreGive(
    mutexHistorico
  );
}
void limparHistorico() {
  if (
    xSemaphoreTake(
      mutexHistorico,
      pdMS_TO_TICKS(100)
    ) != pdTRUE
  ) {
    return;
  }
  for (
    int i = 0;
    i < TAMANHO_HISTORICO;
    i++
  ) {
    historico[i].numero = 0;
    historico[i].tempo = 0;
    historico[i].rotacao = false;
    historico[i].posturaAlterada = false;
    historico[i].imobilidade = false;
    historico[i].quedaConfirmada = false;
  }
  indiceHistorico = 0;
  totalHistorico = 0;
 
  if (persistenciaAtiva) salvarHistoricoPersistente();
  xSemaphoreGive(
    mutexHistorico
  );
  Serial.println();
  Serial.println(
    "HISTORICO LIMPO COM SUCESSO"
  );
  Serial.println();
}
bool inicializarPersistencia() {
  Serial.println();
  Serial.println("Inicializando memoria persistente...");
  if (!preferences.begin(NVS_NAMESPACE, false)) {
    Serial.println("ERRO: nao foi possivel abrir a NVS.");
    persistenciaAtiva = false;
    return false;
  }
  persistenciaAtiva = true;
  Serial.println("NVS aberta com sucesso.");
  return true;
}
bool salvarHistoricoPersistente() {
  if (!persistenciaAtiva) {
    return false;
  }
 
  size_t okVersao = preferences.putUInt(
    "ver",
    NVS_VERSAO
  );
  size_t okIndice = preferences.putInt(
    "idx",
    indiceHistorico
  );
  size_t okTotal = preferences.putInt(
    "total",
    totalHistorico
  );
  size_t okSequencia = preferences.putULong(
    "seq",
    contadorEventos
  );
  size_t bytesSalvos = preferences.putBytes(
    "eventos",
    historico,
    sizeof(historico)
  );
  bool sucesso =
    okVersao > 0 &&
    okIndice > 0 &&
    okTotal > 0 &&
    okSequencia > 0 &&
    bytesSalvos == sizeof(historico);
  if (!sucesso) {
    Serial.println(
      "AVISO: falha ao salvar completamente o historico na NVS."
    );
  }
  return sucesso;
}
bool carregarHistoricoPersistente() {
  if (!persistenciaAtiva) {
    return false;
  }
  uint32_t versao = preferences.getUInt(
    "ver",
    0
  );
  size_t tamanhoSalvo = preferences.getBytesLength(
    "eventos"
  );
  
  if (
    versao != NVS_VERSAO ||
    tamanhoSalvo != sizeof(historico)
  ) {
    Serial.println(
      "Nenhum historico persistente compativel encontrado."
    );
    memset(
      historico,
      0,
      sizeof(historico)
    );
    indiceHistorico = 0;
    totalHistorico = 0;
    contadorEventos = 0;
    salvarHistoricoPersistente();
    Serial.println(
      "Historico persistente inicializado vazio."
    );
    return true;
  }
  size_t bytesLidos = preferences.getBytes(
    "eventos",
    historico,
    sizeof(historico)
  );
  if (bytesLidos != sizeof(historico)) {
    Serial.println(
      "ERRO: tamanho invalido ao carregar historico da NVS."
    );
    return false;
  }
  indiceHistorico = preferences.getInt(
    "idx",
    0
  );
  totalHistorico = preferences.getInt(
    "total",
    0
  );
  contadorEventos = preferences.getULong(
    "seq",
    0
  );
 
  if (
    indiceHistorico < 0 ||
    indiceHistorico >= TAMANHO_HISTORICO
  ) {
    indiceHistorico = 0;
  }
  if (
    totalHistorico < 0 ||
    totalHistorico > TAMANHO_HISTORICO
  ) {
    totalHistorico = 0;
  }
  Serial.println();
  Serial.println("Historico carregado da memoria persistente.");
  Serial.print("Eventos recuperados: ");
  Serial.print(totalHistorico);
  Serial.print("/");
  Serial.println(TAMANHO_HISTORICO);
  Serial.print("Ultimo numero de evento: ");
  Serial.println(contadorEventos);
  Serial.println();
  return true;
}
void fecharPersistencia() {
  if (persistenciaAtiva) {
    preferences.end();
    persistenciaAtiva = false;
  }
}
bool escreverRegistroIMU(uint8_t registro, uint8_t valor) {
  Wire.beginTransmission(IMU_ENDERECO);
  Wire.write(registro);
  Wire.write(valor);
  return Wire.endTransmission() == 0;
}
bool lerRegistradoresIMU(
  uint8_t registroInicial,
  uint8_t* buffer,
  size_t quantidade
) {
  Wire.beginTransmission(IMU_ENDERECO);
  Wire.write(registroInicial);
  if (Wire.endTransmission(false) != 0) {
    return false;
  }
  size_t recebidos = Wire.requestFrom(
    (int)IMU_ENDERECO,
    (int)quantidade,
    (int)true
  );
  if (recebidos != quantidade) {
    while (Wire.available()) {
      Wire.read();
    }
    return false;
  }
  for (size_t i = 0; i < quantidade; i++) {
    buffer[i] = Wire.read();
  }
  return true;
}
bool inicializarIMU() {
  Wire.beginTransmission(IMU_ENDERECO);
  if (Wire.endTransmission() != 0) {
    Serial.println("ERRO: nenhum dispositivo respondeu em 0x68.");
    return false;
  }
  uint8_t id = 0;
  if (!lerRegistradoresIMU(REG_WHO_AM_I, &id, 1)) {
    Serial.println("ERRO: nao foi possivel ler WHO_AM_I.");
    return false;
  }
  Serial.print("WHO_AM_I da IMU: 0x");
  if (id < 0x10) {
    Serial.print("0");
  }
  Serial.println(id, HEX);
  if (id != 0x68 && id != 0x72) {
    Serial.println("AVISO: identificador diferente do testado; prosseguindo por compatibilidade de registradores.");
  }
  if (!escreverRegistroIMU(REG_PWR_MGMT_1, 0x00)) {
    return false;
  }
  delay(100);
  if (!escreverRegistroIMU(REG_SMPLRT_DIV, 0x00)) {
    return false;
  }
  if (!escreverRegistroIMU(REG_CONFIG, 0x04)) {
    return false;
  }
  if (!escreverRegistroIMU(REG_GYRO_CONFIG, 0x08)) {
    return false;
  }
  if (!escreverRegistroIMU(REG_ACCEL_CONFIG, 0x10)) {
    return false;
  }
  delay(50);
  Serial.println("Escala acelerometro: +-8 g");
  Serial.println("Escala giroscopio: +-500 deg/s");
  Serial.println("Filtro digital: configurado");
  return true;
}
bool lerIMU(LeituraIMU &dado) {
  
  uint8_t buffer[14];
  if (!lerRegistradoresIMU(REG_ACCEL_XOUT_H, buffer, sizeof(buffer))) {
    return false;
  }
  auto ler16 = [&](int indice) -> int16_t {
    return (int16_t)(
      ((uint16_t)buffer[indice] << 8) |
      buffer[indice + 1]
    );
  };
  int16_t axRaw = ler16(0);
  int16_t ayRaw = ler16(2);
  int16_t azRaw = ler16(4);
  int16_t gxRaw = ler16(8);
  int16_t gyRaw = ler16(10);
  int16_t gzRaw = ler16(12);
  dado.ax = axRaw / ESCALA_ACEL;
  dado.ay = ayRaw / ESCALA_ACEL;
  dado.az = azRaw / ESCALA_ACEL;
  dado.gx = gxRaw / ESCALA_GIRO;
  dado.gy = gyRaw / ESCALA_GIRO;
  dado.gz = gzRaw / ESCALA_GIRO;
  return true;
}
void taskAmostragem(
  void *pvParameters
) {
  TickType_t ultimoTempo = xTaskGetTickCount();
  unsigned long ultimoErroLeitura = 0;
  while (true) {
    LeituraIMU dado;
    if (lerIMU(dado)) {
      dado.tempo = millis();
      xQueueSend(
        filaIMU,
        &dado,
        0
      );
    } else {
      if (millis() - ultimoErroLeitura >= 1000) {
        ultimoErroLeitura = millis();
        Serial.println("AVISO: falha na leitura da IMU.");
      }
    }
    vTaskDelayUntil(
      &ultimoTempo,
      pdMS_TO_TICKS(INTERVALO_AMOSTRAGEM)
    );
  }
}
void taskProcessamento(
  void *pvParameters
) {
  LeituraIMU amostra;
  unsigned long ultimoPrint = 0;
  unsigned long tempoQuedaLivre = 0;
  unsigned long tempoImpacto = 0;
  unsigned long ultimoDebugJanela = 0;
  unsigned long inicioPosturaAlterada = 0;
  unsigned long inicioImobilidade = 0;
  bool rotacaoDetectada = false;
  bool contandoPosturaAlterada = false;
  bool posturaAlteradaConfirmada = false;
  bool contandoImobilidade = false;
  bool imobilidadeConfirmada = false;
  bool bloqueiaNovaQueda = false;
  while (true) {
    if (
      xQueueReceive(
        filaIMU,
        &amostra,
        portMAX_DELAY
      ) == pdTRUE
    ) {
      float magnitude = sqrt(
        amostra.ax * amostra.ax +
        amostra.ay * amostra.ay +
        amostra.az * amostra.az
      );
      float magnitudeRotacao = sqrt(
        amostra.gx * amostra.gx +
        amostra.gy * amostra.gy +
        amostra.gz * amostra.gz
      );
      if (
        millis() - ultimoPrint >= 500
      ) {
        ultimoPrint =
          millis();
        Serial.print("Mag: ");
        Serial.print(magnitude, 2);
        Serial.print(" g | Giro: ");
        Serial.print(magnitudeRotacao, 1);
        Serial.print(" deg/s | Z: ");
        Serial.print(amostra.az, 2);
        Serial.print(" | Estado: ");
        Serial.println(
          nomeEstado(
            estadoSistema
          )
        );
      }
      if (
        bloqueiaNovaQueda &&
        magnitude >=
        LIMIAR_QUEDA_LIVRE
      ) {
        bloqueiaNovaQueda =
          false;
      }
      switch (
        estadoSistema
      ) {
        case MONITORANDO:
          if (
            !bloqueiaNovaQueda &&
            magnitude <
            LIMIAR_QUEDA_LIVRE
          ) {
            bloqueiaNovaQueda =
              true;
            tempoQuedaLivre =
              millis();
            ultimoDebugJanela =
              millis();
            rotacaoDetectada =
              false;
            posturaAlteradaConfirmada =
              false;
            imobilidadeConfirmada =
              false;
            estadoSistema =
              AGUARDANDO_IMPACTO;
            Serial.println();
            Serial.println(
              ">>> QUEDA LIVRE DETECTADA <<<"
            );
            Serial.println(
              "MONITORANDO -> AGUARDANDO IMPACTO"
            );
            Serial.println();
          }
          break;
        case AGUARDANDO_IMPACTO:
        {
          unsigned long tempoDecorrido =
            millis() - tempoQuedaLivre;
          if (
            magnitudeRotacao >
            LIMIAR_ROTACAO &&
            !rotacaoDetectada
          ) {
            rotacaoDetectada =
              true;
            Serial.println();
            Serial.println(
              ">>> ROTACAO BRUSCA DETECTADA <<<"
            );
            Serial.println();
          }
          if (
            millis() -
            ultimoDebugJanela >=
            250
          ) {
            ultimoDebugJanela =
              millis();
            Serial.print(
              "Janela: "
            );
            Serial.print(
              tempoDecorrido
            );
            Serial.println(
              " ms"
            );
          }
          if (
            magnitude >
            LIMIAR_IMPACTO &&
            tempoDecorrido <
            JANELA_IMPACTO
          ) {
            tempoImpacto =
              millis();
            contandoPosturaAlterada =
              false;
            posturaAlteradaConfirmada =
              false;
            inicioPosturaAlterada =
              0;
            contandoImobilidade =
              false;
            imobilidadeConfirmada =
              false;
            inicioImobilidade =
              0;
            estadoSistema =
              ANALISE_POS_IMPACTO;
            Serial.println();
            Serial.println(
              ">>> IMPACTO DETECTADO <<<"
            );
            Serial.print(
              "Rotacao detectada: "
            );
            Serial.println(
              rotacaoDetectada
              ? "SIM"
              : "NAO"
            );
            Serial.println(
              "AGUARDANDO IMPACTO -> ANALISE POS-IMPACTO"
            );
            Serial.println();
          }
          else if (
            tempoDecorrido >=
            JANELA_IMPACTO
          ) {
            estadoSistema =
              MONITORANDO;
            Serial.println();
            Serial.println(
              ">>> JANELA DE IMPACTO EXPIRADA <<<"
            );
            Serial.println(
              ">>> EVENTO DESCARTADO <<<"
            );
            Serial.println(
              "AGUARDANDO IMPACTO -> MONITORANDO"
            );
            Serial.println();
          }
          break;
        }
        case ANALISE_POS_IMPACTO:
        {
          unsigned long tempoPosImpacto =
            millis() - tempoImpacto;
          if (
            tempoPosImpacto >=
            ATRASO_ANALISE_POSTURA
          ) {
            if (
              fabs(amostra.az) <
              LIMIAR_ORIENTACAO_Z
            ) {
              if (
                !contandoPosturaAlterada
              ) {
                contandoPosturaAlterada =
                  true;
                inicioPosturaAlterada =
                  millis();
                Serial.println(
                  "Possivel postura alterada..."
                );
              }
              if (
                contandoPosturaAlterada &&
                millis() -
                inicioPosturaAlterada >=
                TEMPO_POSTURA_ALTERADA
              ) {
                if (
                  !posturaAlteradaConfirmada
                ) {
                  posturaAlteradaConfirmada =
                    true;
                  Serial.println(
                    ">>> POSTURA ALTERADA CONFIRMADA <<<"
                  );
                }
              }
            }
            else {
              contandoPosturaAlterada = false;
              inicioPosturaAlterada = 0;
              posturaAlteradaConfirmada = false;
            }
            bool magnitudeEstavel =
              fabs(
                magnitude - 1.0
              ) <=
              TOLERANCIA_IMOBILIDADE;
            if (
              magnitudeEstavel
            ) {
              if (
                !contandoImobilidade
              ) {
                contandoImobilidade =
                  true;
                inicioImobilidade =
                  millis();
                Serial.println(
                  "Possivel imobilidade..."
                );
              }
              if (
                contandoImobilidade &&
                millis() -
                inicioImobilidade >=
                TEMPO_IMOBILIDADE
              ) {
                if (
                  !imobilidadeConfirmada
                ) {
                  imobilidadeConfirmada =
                    true;
                  Serial.println(
                    ">>> IMOBILIDADE CONFIRMADA <<<"
                  );
                }
              }
            }
            else {
              if (
                !imobilidadeConfirmada
              ) {
                contandoImobilidade =
                  false;
                inicioImobilidade =
                  0;
              }
            }
          }
          if (
            tempoPosImpacto >=
            JANELA_POS_IMPACTO
          ) {
            Serial.println();
            if (
              rotacaoDetectada &&
              posturaAlteradaConfirmada &&
              imobilidadeConfirmada
            ) {
              registrarEvento(
                rotacaoDetectada,
                posturaAlteradaConfirmada,
                imobilidadeConfirmada,
                true
              );
              estadoSistema =
                ALERTA;
              Serial.println(
                ">>> ROTACAO BRUSCA: SIM <<<"
              );
              Serial.println(
                ">>> POSTURA ALTERADA: SIM <<<"
              );
              Serial.println(
                ">>> IMOBILIDADE: SIM <<<"
              );
              Serial.println(
                ">>> QUEDA CONFIRMADA <<<"
              );
              Serial.println(
                "ANALISE POS-IMPACTO -> ALERTA"
              );
              Serial.println(
                ">>> ALERTA ATIVADO <<<"
              );
            }
            else {
              registrarEvento(
                rotacaoDetectada,
                posturaAlteradaConfirmada,
                imobilidadeConfirmada,
                false
              );
              Serial.println(
                ">>> EVENTO DESCARTADO <<<"
              );
              Serial.print(
                "Rotacao brusca: "
              );
              Serial.println(
                rotacaoDetectada
                ? "SIM"
                : "NAO"
              );
              Serial.print(
                "Postura alterada: "
              );
              Serial.println(
                posturaAlteradaConfirmada
                ? "SIM"
                : "NAO"
              );
              Serial.print(
                "Imobilidade: "
              );
              Serial.println(
                imobilidadeConfirmada
                ? "SIM"
                : "NAO"
              );
              estadoSistema =
                MONITORANDO;
              Serial.println(
                "ANALISE POS-IMPACTO -> MONITORANDO"
              );
            }
            Serial.println();
          }
          break;
        }
        case ALERTA:
          break;
      }
    }
  }
}
void taskControle(
  void *pvParameters
) {
  bool ultimoEstadoBotao =
    HIGH;
  while (true) {
    bool estadoBotao =
      digitalRead(
        PIN_BOTAO
      );
    if (estadoBotao == LOW && ultimoEstadoBotao == HIGH && estadoSistema == ALERTA) {
      cancelarAlerta("botao fisico");
      vTaskDelay(pdMS_TO_TICKS(50));
    }
    ultimoEstadoBotao =
      estadoBotao;
    if (
      estadoSistema == ALERTA
    ) {
      digitalWrite(
        PIN_LED,
        HIGH
      );
      tone(
        PIN_BUZZER,
        1000
      );
    }
    else {
      digitalWrite(
        PIN_LED,
        LOW
      );
      noTone(
        PIN_BUZZER
      );
    }
    vTaskDelay(
      pdMS_TO_TICKS(10)
    );
  }
}
