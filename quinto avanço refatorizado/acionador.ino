#include "Arduino.h"
#include "SPI.h"
#include <U8g2lib.h>
#include <RadioLib.h>

// Definição dos pinos SPI para o módulo LoRa
#define LORA_CLK   SCK     // Pino de clock SPI
#define LORA_MISO  MISO    // Pino MISO SPI
#define LORA_MOSI  MOSI    // Pino MOSI SPI
#define LORA_NSS   SS      // Pino de seleção de escravo SPI (Chip Select)

#define LORA_FREQ  915.0   // Frequência do LoRa em MHz
#define LORA_SF    7       // Fator de espalhamento do LoRa
#define LORA_CR    5       // Taxa de codificação do LoRa

#define STATUS_LED_PIN 2
#define VALVULA1 48     // Pino para a valvula 1
#define VALVULA2 26     // Pino para a valvula 2
#define MOTOR   47      // Pino para o motor
U8G2_SSD1306_128X64_NONAME_F_SW_I2C display(U8G2_R0, 18,17,21);
SX1262 Lora = new Module(8, 14, 12, 13);
volatile bool receiveFlag = false;
volatile bool enableInterrupt = true;

String str;

const char *CMD_ACTIVATE_PREFIX = "A";
const char *LEGACY_CMD_ACTIVATE_PREFIX = "acionar##";
const unsigned long LOOP_DELAY_MS = 20;
const int LORA_TX_POWER_DBM = 14;

bool isActivateCommand(const String &payload);
int parseDelayOption(const String &payload);
void restartReceive();

void setup() {
    Serial.begin(115200);
    pinMode(STATUS_LED_PIN, OUTPUT);
    pinMode(VALVULA1, OUTPUT);
    pinMode(VALVULA2, OUTPUT);
    pinMode(MOTOR, OUTPUT);
    digitalWrite(VALVULA1, LOW);
    digitalWrite(VALVULA2, LOW);
    digitalWrite(MOTOR, LOW);

    display.begin();
    display.setFont(u8g2_font_NokiaSmallPlain_te );

    SPI.begin(LORA_CLK, LORA_MISO, LORA_MOSI, LORA_NSS);
    int state = Lora.begin();
    verifica(state);

    Lora.setDio1Action(setFlag);

    state = Lora.startReceive();
    if (state != RADIOLIB_ERR_NONE) {
        Serial.print(F("Failed to start LoRa reception, code: "));
        Serial.println(state);
        while (true);
    }
}

void setFlag(void) {
    if (!enableInterrupt) {
        return;
    }
    receiveFlag = true;
}

void loop() {
    if (receiveFlag) {
        receiveFlag = false;
        enableInterrupt = false;
        Serial.println(F("Receiving finished: "));
       
        int state = Lora.readData(str);

        if (state == RADIOLIB_ERR_NONE) {
            Serial.print(F("Received data successfully!"));
            Serial.println(str);

            if (isActivateCommand(str)) {
                int delayTime = parseDelayOption(str);
                str = "";
                executarRotina(delayTime);
            }
        } else {
            Serial.print(F("Failed to receive data, code: "));
            Serial.println(state);
        }

        restartReceive();
        enableInterrupt = true;
    } else {
        delay(LOOP_DELAY_MS);
    }
}
void executarRotina(int delayTime) {
  sendCommand("OK");
  Serial.println("OK");
    int delayMillis = 0;
    switch (delayTime) {
        case 1:
            delayMillis = 30000;
            break;
        case 2:
            delayMillis = 60000;
            break;
        case 3:
            delayMillis = 90000;
            break;
        default:
            Serial.println(F("Invalid delay time specified"));
            return;
    }
    int aux = delayMillis;
            display.clearBuffer();
            display.drawStr(0, 10, (String("delay selecionado ") + String(aux)).c_str());
            display.drawStr(0, 20, "abrindo valvula 1");
            display.sendBuffer();



    digitalWrite(VALVULA1, HIGH);

    digitalWrite(STATUS_LED_PIN,HIGH);
    delay(1000);
    digitalWrite(STATUS_LED_PIN,LOW);

    delay(2000);
    digitalWrite(MOTOR, HIGH);
     display.clearBuffer();
            display.drawStr(0, 20, "ligando a bomba");
            display.sendBuffer();

    digitalWrite(STATUS_LED_PIN,HIGH);
    delay(3000);
    digitalWrite(STATUS_LED_PIN,LOW);
    
    delay(delayMillis);
    digitalWrite(VALVULA2, HIGH);
     display.clearBuffer();
            display.drawStr(0, 20, "abrindo valvula 2 e fechando valvula 1");
            display.sendBuffer();
    digitalWrite(VALVULA1, LOW);
    digitalWrite(STATUS_LED_PIN,HIGH);
    delay(5000);
    digitalWrite(STATUS_LED_PIN,LOW);
 
    delay(delayMillis);
    digitalWrite(MOTOR, LOW);
    delay(2000);
           display.clearBuffer();    
            display.drawStr(0, 20, "desligando bomba e fechando valvula 2");
            display.sendBuffer();
    digitalWrite(VALVULA2, LOW);

      display.clearBuffer();
      display.drawStr(0, 10, "esperando!");
       display.sendBuffer();
str="";


            
}

void verifica(int state) {
  if (state == RADIOLIB_ERR_NONE) {
    Serial.println(F("LoRa Initialized!"));
    display.clearBuffer();
    display.drawStr(0, 10, "LoRa Initialized! acionador");
    display.sendBuffer();
    } else {
    Serial.print(F("LoRa failed, code "));
    Serial.println(state);
    display.clearBuffer();
    display.drawStr(0, 10, (String("LoRa failed, code: ") + state).c_str());
    display.sendBuffer();
    while (true)
      ;
  }

  if (Lora.setFrequency(915.0) == RADIOLIB_ERR_INVALID_FREQUENCY) {
    Serial.println(F("Selected value is invalid for FREQUENCY"));
    while (true)
      ;
  }
  if (Lora.setSpreadingFactor(7) == RADIOLIB_ERR_INVALID_SPREADING_FACTOR) {
    Serial.println(F("Selected value is invalid for SPREADING_FACTOR!"));
    while (true)
      ;
  }
  if (Lora.setCodingRate(5) == RADIOLIB_ERR_INVALID_CODING_RATE) {
    Serial.println(F("Selected value is invalid for CODING_RATE!"));
    while (true)
      ;
  }
  if (Lora.setOutputPower(LORA_TX_POWER_DBM) == RADIOLIB_ERR_INVALID_OUTPUT_POWER) {
    Serial.println(F("Selected value is invalid for OUTPUT_POWER!"));
    while (true)
      ;
  }
}
void sendCommand(String command) {
    int state = Lora.transmit(command);
    if (state == RADIOLIB_ERR_NONE) {
        Serial.println(F("comando enviado"));
    } else {
        Serial.print(F("erro ao acionar valvulas"));
        Serial.println(state);
    }
    restartReceive();
}

bool isActivateCommand(const String &payload) {
    return payload.startsWith(CMD_ACTIVATE_PREFIX) || payload.startsWith(LEGACY_CMD_ACTIVATE_PREFIX);
}

int parseDelayOption(const String &payload) {
    if (payload.startsWith(CMD_ACTIVATE_PREFIX)) {
        return payload.substring(String(CMD_ACTIVATE_PREFIX).length()).toInt();
    }

    return payload.substring(String(LEGACY_CMD_ACTIVATE_PREFIX).length()).toInt();
}

void restartReceive() {
    int state = Lora.startReceive();
    if (state != RADIOLIB_ERR_NONE) {
        Serial.print(F("Failed to restart LoRa reception, code: "));
        Serial.println(state);
    }
}

