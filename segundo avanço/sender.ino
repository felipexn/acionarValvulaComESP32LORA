#include "Arduino.h"
#include "SPI.h"
#include <U8g2lib.h>
#include <RadioLib.h>

#include <Wire.h>

// Definição dos pinos SPI para o módulo LoRa
#define LORA_CLK   SCK     // Pino de clock SPI
#define LORA_MISO  MISO    // Pino MISO SPI
#define LORA_MOSI  MOSI    // Pino MOSI SPI
#define LORA_NSS   SS      // Pino de seleção de escravo SPI (Chip Select)


U8G2_SSD1306_128X64_NONAME_F_SW_I2C display(U8G2_R0, 18,17,21);   // All Boards without Reset of the Display

SX1262 Lora = new Module(8,14,12,13);

unsigned int counter = 0;

bool Atuar = false;
int transmissionState = RADIOLIB_ERR_NONE;



void verifica (int state);


void setup(void) {

    Serial.begin(115200);
    display.begin();
    display.setFont(u8g2_font_NokiaSmallPlain_te );

    pinMode(LED, OUTPUT);
    

    SPI.begin(LORA_CLK, LORA_MISO, LORA_MOSI, LORA_NSS);

    int state = Lora.begin();
    verifica(state);



}

void loop(){
  
	
  Serial.println(F("Start transmitting data")) ;
    

    if(counter%2==0){
		String sendData = " HIGH " ;
    transmissionState = Lora.startTransmit(sendData) ;
     Serial.println(F("high"));
    }
    else{
      String sendData = " LOW " ;
    transmissionState = Lora.startTransmit(sendData) ;
    Serial.println(F("low")) ;
    }

  



  display.drawStr(0, 10, "lora data show");
			display.drawStr(0, 20, String("numero  " + (String)(counter)).c_str());
			display.sendBuffer();
      
			counter++;

    delay(5000);


}

void verifica (int state){
       if (state == RADIOLIB_ERR_NONE){
        Serial.println(F("LoRa Initialized!"));
        display.clearBuffer();
        display.drawStr(0, 10, "LoRa Initialized!");
        display.sendBuffer();
    }else{
        Serial.print(F("LoRa failed, code "));
        Serial.println(state);
        display.clearBuffer();
        display.drawStr(0, 10, "LoRa failed, code:" + state);
        display.sendBuffer();
        while (true);
    }

    if (Lora.setFrequency(915.0) == RADIOLIB_ERR_INVALID_FREQUENCY){
        Serial.println(F("Selected value is invalid for FREQUENCY"));
        while (true);
    }
    if (Lora.setSpreadingFactor(7) == RADIOLIB_ERR_INVALID_SPREADING_FACTOR) {
        Serial.println(F("Selected value is invalid for SPREADING_FACTOR!"));
        while (true);
    }
    if (Lora.setCodingRate(5) == RADIOLIB_ERR_INVALID_CODING_RATE){
        Serial.println(F("Selected value is invalid for CODING_RATE!"));
        while (true);
    }
}



