#include <SPI.h>
#include <Wire.h>
#include <RadioLib.h>

// Definição dos pinos SPI para o módulo LoRa SX1276/77/78/79
#define LORA_SCK 5    // GPIO5  -- SX127x's SCK
#define LORA_MISO 19  // GPIO19 -- SX127x's MISO
#define LORA_MOSI 27  // GPIO27 -- SX127x's MOSI
#define LORA_SS 18    // GPIO18 -- SX127x's CS
#define LORA_RST 14   // GPIO14 -- SX127x's RESET
#define LORA_DIO0 26  // GPIO26 -- SX127x's IRQ (Interrupt Request)

// Instancia do módulo LoRa
SX1276 LoRa = new Module(LORA_SS, LORA_DIO0, LORA_RST);

#define STATUS_LED_PIN 25
#define HUMIDITY_SENSOR_PIN 12  // GPIO12 para leitura analogica

const char *HUMIDITY_PREFIX = "H:";
const int LORA_TX_POWER_DBM = 10;
const uint8_t HUMIDITY_CHANGE_THRESHOLD = 2;
const unsigned long SAMPLE_INTERVAL_MS = 30000;
const unsigned long FORCE_SEND_INTERVAL_MS = 600000;

int lastSentHumidity = -1;
unsigned long lastSampleAt = 0;
unsigned long lastSendAt = 0;



void setup() {
  Serial.begin(115200);

  pinMode(STATUS_LED_PIN, OUTPUT);
  digitalWrite(STATUS_LED_PIN, LOW);

  SPI.begin(LORA_SCK, LORA_MISO, LORA_MOSI, LORA_SS);

  int state = LoRa.begin();
  if (state == RADIOLIB_ERR_NONE) {
    Serial.println("LoRa initialized successfully!");
  } else {
    Serial.print("Failed to initialize LoRa, code ");
    Serial.println(state);
    while (true);
  }
  if (LoRa.setFrequency(915.0) == RADIOLIB_ERR_INVALID_FREQUENCY ||
        LoRa.setSpreadingFactor(7) == RADIOLIB_ERR_INVALID_SPREADING_FACTOR ||
        LoRa.setCodingRate(5) == RADIOLIB_ERR_INVALID_CODING_RATE ||
        LoRa.setOutputPower(LORA_TX_POWER_DBM) == RADIOLIB_ERR_INVALID_OUTPUT_POWER) {
        Serial.println(F("Invalid LoRa configuration!"));
        while (true);
  }
}

float readHumidityPercentage() {
  int sensorValue = analogRead(HUMIDITY_SENSOR_PIN);
  return (sensorValue / 4095.0f) * 100.0f;
}

void loop() {
  unsigned long now = millis();
  if (lastSampleAt != 0 && now - lastSampleAt < SAMPLE_INTERVAL_MS) {
    delay(100);
    return;
  }
  lastSampleAt = now;

  int umidade = round(readHumidityPercentage());
  bool changedEnough = lastSentHumidity < 0 || abs(umidade - lastSentHumidity) >= HUMIDITY_CHANGE_THRESHOLD;
  bool forceSend = now - lastSendAt >= FORCE_SEND_INTERVAL_MS;

  Serial.println(umidade);

  if (!changedEnough && !forceSend) {
    return;
  }

  String message = String(HUMIDITY_PREFIX) + String(umidade);
  int state = LoRa.transmit(message);
  if (state == RADIOLIB_ERR_NONE) {
    Serial.println("Message sent successfully!");
    lastSentHumidity = umidade;
    lastSendAt = now;
    digitalWrite(STATUS_LED_PIN, HIGH);
    delay(50);
    digitalWrite(STATUS_LED_PIN, LOW);
  } else {
    Serial.print("Failed to send message, code ");
    Serial.println(state);
  }
}

