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

#define VALVULA1 48     // Pino para a valvula 1 
#define VALVULA2 26     // Pino para a valvula 2 
#define MOTOR   47      // Pino para o motor 

U8G2_SSD1306_128X64_NONAME_F_SW_I2C display(U8G2_R0, 18,17,21);   // All Boards without Reset of the Display

SX1262 Lora = new Module(8, 14, 12, 13);

volatile bool receiveFlag = false;

 String str;

void setup() {
    Serial.begin(115200);
    pinMode(LED, OUTPUT);
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

    // Set the function that will be called when a new packet is received
    Lora.setDio1Action(setFlag);

    // Start listening for LoRa packets
    state = Lora.startReceive();
    if (state != RADIOLIB_ERR_NONE) {
        Serial.print(F("Failed to start LoRa reception, code: "));
        Serial.println(state);
        while (true);
    }
}

// Disable interrupt when it's not needed
volatile bool eenableInterrupt = true;

// This function is called when a complete packet is received by the module
void setFlag(void) {
    if (!enableInterrupt) {
        return;
    }
    receiveFlag = true;
}

void loop() {
    if (receiveFlag) {
        receiveFlag = false;
        eenableInterrupt = false;
        Lora.startReceive();
        Serial.println(F("Receiving finished: "));
       
        int state = Lora.readData(str);

        if (state == RADIOLIB_ERR_NONE) {
            Serial.print(F("Received data successfully!"));
            //Serial.println(F("[SX1262] Data:\t\t"));
            Serial.println(str);

            // Verifica o comando recebido
            if (str.startsWith(" acionar##")) {
               
               state=0;
                int delayTime = str.substring(10).toInt(); // Extrai o número inteiro após "acionar\\"
                str="";
                executarRotina(delayTime);
               //delay(1000); 
               
            }
        } else {
            Serial.print(F("Failed to receive data, code: "));
            Serial.println(state);
        }

        delay(10000); // Delay for stability
        eenableInterrupt = true;
    } else {
        Serial.println(F("Nothing to receive"));
        delay(1000);
    }
}

void executarRotina(int delayTime) {
  sendCommand("recebido ");
  Serial.println("recebido");
    int delayMillis = 0;
    
    switch (delayTime) {
        case 1:
            delayMillis = 30000; // 30 segundos
            break;
        case 2:
            delayMillis = 60000; // 60 segundos
            break;
        case 3:
            delayMillis = 90000; // 90 segundos
            break;
        default:
            Serial.println(F("Invalid delay time specified"));
            return;
    }
    int aux = delayMillis;
            display.clearBuffer();
            display.drawStr(0, 10, String("delay selecionado " + (String)(aux)).c_str());
            display.drawStr(0, 20, "abrindo valvula 1");
            display.sendBuffer();



    // rotina de ligar motor e as válvulas
    digitalWrite(VALVULA1, HIGH);  // abrir valvula 1

    digitalWrite(LED,HIGH);
    delay(1000);
    digitalWrite(LED,LOW);

    delay(2000);                       // delay de 2 segundos
    digitalWrite(MOTOR, HIGH);     // ligar motor
     display.clearBuffer();
            display.drawStr(0, 20, "ligando a bomba");
            display.sendBuffer();

    digitalWrite(LED,HIGH);
    delay(3000);
    digitalWrite(LED,LOW);
    
    delay(delayMillis);                // delay do tempo especificado para regar tudo no primeiro plantio
    digitalWrite(VALVULA2, HIGH);  // abrir valvula 2
     display.clearBuffer();
            display.drawStr(0, 20, "abrindo valvula 2 e fechando valvula 1");
            display.sendBuffer();
    digitalWrite(VALVULA1, LOW);   // fechar valvula 1
    digitalWrite(LED,HIGH);
    delay(5000);
    digitalWrite(LED,LOW);
 
    delay(delayMillis);                // delay do tempo especificado para regar tudo no segundo plantio
    digitalWrite(MOTOR, LOW);     // desligar motor
    delay(2000);
           display.clearBuffer();    
            display.drawStr(0, 20, "desligando bomba e fechando valvula 2");
            display.sendBuffer();
    digitalWrite(VALVULA2, LOW);   // fechar valvula 2

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
    display.drawStr(0, 10, "LoRa failed, code:" + state);
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
}
void sendCommand(String command) {
    int state = Lora.transmit(command);
    if (state == RADIOLIB_ERR_NONE) {
        Serial.println(F("comando enviado"));
    } else {
        Serial.print(F("erro ao acionar valvulas"));
        Serial.println(state);
    }
}

