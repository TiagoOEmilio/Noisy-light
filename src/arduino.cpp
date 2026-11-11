#include <Arduino.h>
#include <SoftwareSerial.h>
#include <DFRobotDFPlayerMini.h>

// ---------------- PINOS ----------------

const int botao = 2;

const int ledVermelho = 13;
const int ledVerde = 12;

const int rgbVermelho = 10;
const int rgbVerde = 11;

const int buzzer = 9;
const int motor = 8;

const int pinoBusy = 7;
const int sensor = A0;

// Comunicação com DFPlayer
SoftwareSerial comunicacaoDF(4, 5);
DFRobotDFPlayerMini player;

// ---------------- TEMPOS ----------------

const unsigned long duracaoEstado = 10000;
const unsigned long intervaloBip = 500;

unsigned long duracaoBip = 100;

const unsigned long duracaoVibracao = 150;

const unsigned long intervaloMedicao = 100;
const unsigned long intervaloSerial = 3000;

// ---------------- ESTADOS ----------------

bool faseVerde = false;

unsigned long inicioEstado = 0;
unsigned long ultimaMedicao = 0;
unsigned long ultimaImpressao = 0;

bool ultimoEstadoBotao = HIGH;

bool vibrando = false;
unsigned long inicioVibracao = 0;

bool bipLigado = false;
unsigned long inicioBip = 0;
unsigned long ultimoBip = 0;

bool dfDisponivel = false;

// ---------------- SENSOR DE RUÍDO ----------------

int amplitudeRuido = 0;
int nivelSom = 0;

const int amplitudeMaxima = 80;

// Intensidade do RGB
const int intensidadeVermelho = 255;
const int intensidadeVerde = 120;

// ---------------- CONFIGURAÇÃO ----------------

void setup() {

  pinMode(botao, INPUT_PULLUP);

  pinMode(motor, OUTPUT);
  pinMode(buzzer, OUTPUT);

  pinMode(ledVermelho, OUTPUT);
  pinMode(ledVerde, OUTPUT);

  pinMode(rgbVermelho, OUTPUT);
  pinMode(rgbVerde, OUTPUT);

  pinMode(pinoBusy, INPUT);

  digitalWrite(motor, LOW);
  digitalWrite(buzzer, LOW);

  Serial.begin(9600);
  comunicacaoDF.begin(9600);

  if (player.begin(comunicacaoDF)) {
    dfDisponivel = true;
    player.volume(25);
    Serial.println("DFPlayer conectado!");
  } else {
    Serial.println("DFPlayer nao encontrado.");
  }

  inicioEstado = millis();
  ultimoBip = millis();

  atualizarSemaforo();

  Serial.println("Sistema iniciado.");
}

// ---------------- LOOP PRINCIPAL ----------------

void loop() {

  atualizarEstado();
  atualizarSemaforo();
  atualizarBotao();
  atualizarVibracao();
  atualizarBipes();
  atualizarRuido();
  monitorSerial();

}

// ---------------- SEMÁFORO ----------------

void atualizarEstado() {

  unsigned long agora = millis();

  if (agora - inicioEstado >= duracaoEstado) {

    inicioEstado = agora;
    faseVerde = !faseVerde;
    ultimoBip = agora;

  }
}

void atualizarSemaforo() {

  if (faseVerde) {

    digitalWrite(ledVermelho, LOW);
    digitalWrite(ledVerde, HIGH);

    analogWrite(rgbVermelho, 0);
    analogWrite(rgbVerde, intensidadeVerde);

  } else {

    digitalWrite(ledVermelho, HIGH);
    digitalWrite(ledVerde, LOW);

    analogWrite(rgbVermelho, intensidadeVermelho);
    analogWrite(rgbVerde, 0);

  }
}

// ---------------- BOTÃO ----------------

void atualizarBotao() {

  bool estadoBotao = digitalRead(botao);

  if (ultimoEstadoBotao == HIGH &&
      estadoBotao == LOW) {

    if (!vozTocando()) {

      iniciarVibracao();
      anunciarTempoRestante();

    }
  }

  ultimoEstadoBotao = estadoBotao;
}

// ---------------- VIBRAÇÃO ----------------

void iniciarVibracao() {

  digitalWrite(motor, HIGH);

  vibrando = true;
  inicioVibracao = millis();

}

void atualizarVibracao() {

  if (vibrando &&
      millis() - inicioVibracao >= duracaoVibracao) {

    digitalWrite(motor, LOW);
    vibrando = false;

  }
}

// ---------------- DFPLAYER ----------------

bool vozTocando() {

  if (!dfDisponivel) {
    return false;
  }

  return digitalRead(pinoBusy) == LOW;

}

void anunciarTempoRestante() {

  if (!dfDisponivel || vozTocando()) {
    return;
  }

  unsigned long decorrido = millis() - inicioEstado;

  int restante =
    (duracaoEstado - decorrido + 999) / 1000;

  restante = constrain(restante, 1, 10);

  int faixa = 11 - restante;

  player.playMp3Folder(faixa);

}

// ---------------- BIPES ----------------

void atualizarBipes() {

  unsigned long agora = millis();

  if (vozTocando()) {

    noTone(buzzer);
    bipLigado = false;

    return;

  }

  if (!bipLigado &&
      agora - ultimoBip >= intervaloBip) {

    ultimoBip = agora;
    inicioBip = agora;
    bipLigado = true;

    duracaoBip = map(
      nivelSom,
      0,
      120,
      20,
      100
    );

    tone(buzzer, 2000);

  }

  if (bipLigado &&
      agora - inicioBip >= duracaoBip) {

    noTone(buzzer);
    bipLigado = false;

  }
}

// ---------------- MEDIÇÃO DO RUÍDO ----------------

int medirRuido() {

  int maior = 0;
  int menor = 1023;

  unsigned long inicio = millis();

  while (millis() - inicio < 50) {

    int leitura = analogRead(sensor);

    if (leitura > maior) {
      maior = leitura;
    }

    if (leitura < menor) {
      menor = leitura;
    }

  }

  return maior - menor;

}

void atualizarRuido() {

  unsigned long agora = millis();

  if (vozTocando() || bipLigado) {
    return;
  }

  if (agora - ultimaMedicao >= intervaloMedicao) {

    ultimaMedicao = agora;

    int leitura = medirRuido();

    amplitudeRuido =
      (amplitudeRuido * 2 + leitura) / 3;

    nivelSom = map(
      amplitudeRuido,
      0,
      amplitudeMaxima,
      0,
      120
    );

    nivelSom = constrain(nivelSom, 0, 120);

  }
}

// ---------------- MONITOR SERIAL ----------------

void monitorSerial() {

  unsigned long agora = millis();

  if (agora - ultimaImpressao >= intervaloSerial) {

    ultimaImpressao = agora;

    unsigned long decorrido = millis() - inicioEstado;

    int restante =
      (duracaoEstado - decorrido + 999) / 1000;

    restante = constrain(restante, 0, 10);

    Serial.println("-------------------------");

    if (faseVerde) {
      Serial.println("Semaforo: VERDE");
    } else {
      Serial.println("Semaforo: VERMELHO");
    }

    Serial.print("Tempo restante: ");
    Serial.print(restante);
    Serial.println(" segundos");

    Serial.print("Amplitude do microfone: ");
    Serial.println(amplitudeRuido);

    Serial.print("Nivel de ruido estimado: ");
    Serial.print(nivelSom);
    Serial.println(" dB (escala relativa)");

    Serial.print("Duracao do bip: ");
    Serial.print(duracaoBip);
    Serial.println(" ms");

    Serial.println("-------------------------");

  }
}