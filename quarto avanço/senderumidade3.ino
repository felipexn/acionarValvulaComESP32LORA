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
int contador;

// Definição do pino do LED embutido
#define LED_BUILTIN 25
#define HUMIDITY_SENSOR_PIN 12  // GPIO34 para leitura analógica



void setup() {
  // Inicializa a comunicação serial
  Serial.begin(115200);
  
  // Configura o pino do LED como saída
  pinMode(LED_BUILTIN, OUTPUT);

  // Inicializa a SPI
  SPI.begin(LORA_SCK, LORA_MISO, LORA_MOSI, LORA_SS);

  // Inicializa o módulo LoRa
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
        LoRa.setCodingRate(5) == RADIOLIB_ERR_INVALID_CODING_RATE) {
        Serial.println(F("Invalid LoRa configuration!"));
        while (true);
    }
}

void loop() {
  
  
    int sensorValue = analogRead(HUMIDITY_SENSOR_PIN);
  float umidade = (sensorValue / 4095.0) * 100.0;  // Conversão para porcentagem (0-100%)

  Serial.println(sensorValue);
  Serial.println(umidade);
  // Envia uma mensagem via LoRa
   String message = " Umidade: " + String(umidade, 2) + "%";
  int state = LoRa.transmit(message);
  if (state == RADIOLIB_ERR_NONE) {
    Serial.println("Message sent successfully!");
  } else {
    Serial.print("Failed to send message, code ");
    Serial.println(state);
  }
  // Aguarda 1 segundo
  delay(3000);
}

