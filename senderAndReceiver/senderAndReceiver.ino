#include "Arduino.h"
#include "SPI.h"
#include <U8g2lib.h>
#include <RadioLib.h>
#include "images.h"
#include <Wire.h>

// Definição dos pinos SPI para o módulo LoRa
#define LORA_CLK   SCK     // Pino de clock SPI
#define LORA_MISO  MISO    // Pino MISO SPI
#define LORA_MOSI  MOSI    // Pino MOSI SPI
#define LORA_NSS   SS      // Pino de seleção de escravo SPI (Chip Select)


U8G2_SSD1306_128X64_NONAME_F_SW_I2C display(U8G2_R0, 18,17,21);   // All Boards without Reset of the Display

SX1262 Lora = new Module(8,14,12,13);

unsigned int counter = 0;

bool transmitFlag = false;
int transmissionState = RADIOLIB_ERR_NONE;

volatile bool operationDone = false;

void logo(){
    display.clearBuffer();
    display.drawXBM(0,5, logo_width, logo_height,(const unsigned char *)logo_bits);
    display.sendBuffer();
}

void setFlag(void) {
	operationDone = true;
	Serial.println("operationDone = true");
}

void setup(void) {
    Serial.begin(115200);
    display.begin();
    display.setFont(u8g2_font_NokiaSmallPlain_te );

    pinMode(LED, OUTPUT);
    logo();
    delay(3000);
    display.clear();

    SPI.begin(LORA_CLK, LORA_MISO, LORA_MOSI, LORA_NSS);

    int state = Lora.begin();

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
    // set the function that will be called
  	// when new packet is received
 	Lora.setDio1Action(setFlag);

}

void loop(){
	if(!operationDone){
		delay(100);
		return;
	}

	int16_t rssi = 0;

	operationDone = false;

	if(!transmitFlag){
		Serial.println(F("Transmission finished: ")) ;
		transmitFlag = false;

		if (transmissionState == RADIOLIB_ERR_NONE){
			Serial.print(F("with success!"));
		}
		else{
			Serial.print(F("with error: "));
			Serial.print(transmissionState);
		}

		Serial.println(F("Start receing data")) ;
		transmitFlag = false;
		Lora.startReceive();
	}
	else{
		Serial.println(F("Receiving finished: ")) ;

		String str;
      	int state = Lora.readData(str);

		if (state == RADIOLIB_ERR_NONE){
			Serial.print(F("with success!"));
			Serial.println(F("[SX1262] Data:\t\t"));
        	Serial.println(str);

			rssi = Lora.getRSSI();
			int16_t rssiDetection = abs(rssi);

			if(rssiDetection < 65){
				digitalWrite(LED, HIGH);  
			}
			else{
				digitalWrite(LED, LOW);  
			}

			display.clearBuffer();
			
			display.drawStr(0, 10, "lora data show");
			display.drawStr(0, 20, String("R_data: " + str).c_str());
			display.drawStr(0, 30, String("R_size: " + (String)(str.length()) + " R_rssi:" + (String)(rssiDetection)).c_str());

			display.drawStr(0, 50, String("sent num: " + (String)(counter-1)).c_str());
			display.sendBuffer();
			delay(100); 

		}
		else{
			Serial.print(F("with error"));
		}

		delay(1000);		

		Serial.println(F("Start transmitting data")) ;
		String sendData = "hello #" + String(counter++) + " Rssi:" + (String)(rssi);
    transmissionState = Lora.startTransmit(sendData) ; 

      	transmitFlag = true;
	}
}


