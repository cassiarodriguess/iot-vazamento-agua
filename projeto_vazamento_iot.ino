
#include <WiFi.h>
#include <PubSubClient.h>
#include "HX711.h"

// Configurações do Wi-Fi
const char* ssid = "SEU_SSID";
const char* password = "SUA_SENHA";

// Configurações do MQTT
const char* mqtt_server = "broker.hivemq.com";  // Exemplo público
const int mqtt_port = 1883;
const char* topic_alerta = "iot/vazamento/alerta";

WiFiClient espClient;
PubSubClient client(espClient);

// Pinos dos sensores e atuadores
const int fluxoPin = 4;          // Saída do sensor YF-S201
const int relePin = 14;          // Relé que aciona a válvula
const int hx711_dout = 5;        // DOUT do HX711
const int hx711_sck = 18;        // SCK do HX711

volatile int pulsos = 0;
unsigned long tempoAnterior = 0;
float fluxo_L_por_minuto = 0;

HX711 balanca;

void IRAM_ATTR contarPulso() {
  pulsos++;
}

void setup_wifi() {
  delay(10);
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
  }
}

void reconnect() {
  while (!client.connected()) {
    if (client.connect("ESP32Client")) {
      client.publish(topic_alerta, "ESP32 conectado ao MQTT");
    } else {
      delay(5000);
    }
  }
}

void setup() {
  Serial.begin(115200);
  pinMode(fluxoPin, INPUT_PULLUP);
  pinMode(relePin, OUTPUT);
  attachInterrupt(digitalPinToInterrupt(fluxoPin), contarPulso, RISING);

  balanca.begin(hx711_dout, hx711_sck);
  setup_wifi();
  client.setServer(mqtt_server, mqtt_port);
}

void loop() {
  if (!client.connected()) reconnect();
  client.loop();

  unsigned long tempoAtual = millis();
  if (tempoAtual - tempoAnterior >= 1000) {
    detachInterrupt(digitalPinToInterrupt(fluxoPin));
    fluxo_L_por_minuto = (pulsos / 7.5);
    pulsos = 0;
    tempoAnterior = tempoAtual;

    long leituraPressao = balanca.read_average(5);

    Serial.print("Fluxo: ");
    Serial.print(fluxo_L_por_minuto);
    Serial.print(" L/min - Pressão bruta: ");
    Serial.println(leituraPressao);

    // Lógica de detecção de vazamento (exemplo simples)
    if (fluxo_L_por_minuto > 0.2 && leituraPressao < 10000) {
      digitalWrite(relePin, HIGH);  // Fecha válvula
      client.publish(topic_alerta, "Vazamento detectado! Válvula fechada.");
    } else {
      digitalWrite(relePin, LOW);   // Mantém válvula aberta
    }

    attachInterrupt(digitalPinToInterrupt(fluxoPin), contarPulso, RISING);
  }
}
