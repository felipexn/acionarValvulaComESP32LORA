#include "Arduino.h"
#include "SPI.h"
#include <U8g2lib.h>
#include <RadioLib.h>
#include <WiFi.h> 
#include <Wire.h>
#include <NTPClient.h>
#include <WiFiUdp.h>
#include <WiFiClient.h>
#include <WebServer.h>
#include <TimeLib.h>

//Definição do login e senha do wifi
//#define EAP_IDENTIDADE "felipe@unifesspa.edu.br" // SSID da sua rede WiFi
#define EAP_USUARIO "Bella bronze" // SSID da sua rede WiFi
#define EAP_SENHA "delmadepilacao" // Senha da sua rede WiFi

//#define EAP_USUARIO "HUAWEI P10 lite" // SSID da sua rede WiFi
//#define EAP_SENHA "leslye5412" // Senha da sua rede WiFi

const char *ntpServer = "pool.ntp.org"; // Servidor NTP para sincronização de hora
const long gmtOffset_sec = -3 * 3600;           // Deslocamento de fuso horário em segundos (0 para UTC)
const int daylightOffset_sec = 0;       // Deslocamento de horário de verão em segundos (0 para nenhum)
//const char* ssid = "UNIFESSPA";

WebServer server(80);    // Cria um servidor na porta 80


// Definição dos pinos SPI para o módulo LoRa
#define LORA_CLK   SCK     // Pino de clock SPI
#define LORA_MISO  MISO    // Pino MISO SPI
#define LORA_MOSI  MOSI    // Pino MOSI SPI
#define LORA_NSS   SS      // Pino de seleção de escravo SPI (Chip Select)


#define LORA_FREQ  915.0   // Frequência do LoRa em MHz
#define LORA_SF    7       // Fator de espalhamento do LoRa
#define LORA_CR    5       // Taxa de codificação do LoRa

U8G2_SSD1306_128X64_NONAME_F_SW_I2C display(U8G2_R0, 18,17,21);   // All Boards without Reset of the Display

SX1262 Lora = new Module(8,14,12,13);

volatile bool transmitFlag = false;

void verifica (int state);

void sendCommand(String command);

unsigned long lastNtpUpdate = 0;
const unsigned long ntpUpdateInterval = 600000; // Intervalo de atualização do NTP em milissegundos (10 minutos)


WiFiUDP ntpUDP;
NTPClient timeClient(ntpUDP, ntpServer, gmtOffset_sec, daylightOffset_sec);

int m=0;
int h=0;
int delayOption = 1; 

void handleRoot() {
    server.send(200, "text/html", "<h1>Definir Hora e Delay</h1><form action='/setTime' method='post'>Hora: <input type='text' name='hour'><br>Minuto: <input type='text' name='minute'><br>Delay: <select name='delay'><option value='1'>30 segundos</option><option value='2'>60 segundos</option><option value='3'>90 segundos</option></select><br><input type='submit' value='Definir'></form>");
}

void handleSetTime() {
    String hour = server.arg("hour");
    String minute = server.arg("minute");
    String delay = server.arg("delay");

    // Converte hora, minuto e delay para inteiros
    h = hour.toInt();
    m = minute.toInt();
    delayOption = delay.toInt();

    // Configura o relógio interno do ESP32 com a hora fornecida
    setTime(h, m, 0, 0, 0, 0); // Ajusta a hora no ESP32

    // Exibe a hora definida no monitor serial
    Serial.print("Hora definida: ");
    Serial.print(h);
    Serial.print(":");
    Serial.println(m);

    // Exibe o delay definido no monitor serial
    Serial.print("Delay definido: ");
    Serial.println(delayOption);

    server.send(200, "text/plain", "Hora e delay definidos com sucesso!");
}

void setup() {
    Serial.begin(115200);
    display.begin();
    display.setFont(u8g2_font_NokiaSmallPlain_te );

    pinMode(LED, OUTPUT);

    SPI.begin(LORA_CLK, LORA_MISO, LORA_MOSI, LORA_NSS);

    int state = Lora.begin();
    verifica(state);

     // Inicialização WiFi
     
    WiFi.begin(EAP_USUARIO,EAP_SENHA);
    Serial.print("Conectando ao WiFi");

    // Aguarda a conexão com o WiFi
    while (WiFi.status() != WL_CONNECTED) {
        delay(500);
        Serial.print(".");
    }
        
        display.drawStr(0, 20, ("wifi conectado na rede " + String(EAP_USUARIO)).c_str());
        display.sendBuffer();

    Serial.println("");
    Serial.println("Conectado ao WiFi com sucesso");
    Serial.println(WiFi.localIP());
    // Inicialização do cliente NTP
    timeClient.begin();
    timeClient.update(); // Obtem a hora atual
    configTime(gmtOffset_sec, daylightOffset_sec, ntpServer);

    server.on("/", HTTP_GET, handleRoot);
    server.on("/setTime", HTTP_POST, handleSetTime);
    server.begin();


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
volatile bool enableInterruptt = true;

// This function is called when a complete packet is received by the module
void setFlag(void) {
    if (!enableInterruptt) {
        return;
    }
    transmitFlag = true;
}

void loop() {
  

    
    unsigned long currentMillis = millis();

    // Atualiza o tempo do servidor NTP se o intervalo definido for atingido
    if (currentMillis - lastNtpUpdate >= ntpUpdateInterval) {
        timeClient.update();
        lastNtpUpdate = currentMillis;
    }

    // Obtém a hora atual do servidor NTP
    int currentHour = timeClient.getHours();
    int currentMinute = timeClient.getMinutes();
    Serial.print("Hora agora ");
    Serial.print(currentHour);
    Serial.print(":");
    Serial.println(currentMinute);

    Serial.print("Hora definida: ");
    Serial.print(h);
    Serial.print(":");
    Serial.println(m);
    
   

    if (transmitFlag) {
        transmitFlag = false;
        enableInterruptt = false;
        Lora.startReceive();
        Serial.println(F("Receiving finished: "));
        String str;
        int state = Lora.readData(str);

        if (state == RADIOLIB_ERR_NONE) {
            Serial.print(F("Received data successfully!"));
            Serial.println(str);

            if (str.startsWith(" umidade")) {
                int umidade = str.substring(8).toInt();
                digitalWrite(LED, umidade > 50 ? HIGH : LOW);}

            int16_t rssi = Lora.getRSSI();
            int16_t rssiDetection = abs(rssi);

            display.clearBuffer();
            display.drawStr(0, 10, "LoRa Data:");
            display.drawStr(0, 20, String("Data: " + str).c_str());
            display.drawStr(0, 30, String("Size: " + (String)(str.length()) + " RSSI: " + (String)(rssiDetection)).c_str());
            display.sendBuffer();
        } else {
            Serial.print(F("Failed to receive data, code: "));
            Serial.println(state);
        }

        delay(1000); // Delay for stability
        enableInterruptt = true;
    } else {
        Serial.println(F("Nothing to receive"));
        delay(1000);
    }

     // Verifica se a hora atual é igual à hora definida pelo usuário
    if ((currentHour) == h && currentMinute == m) {
        digitalWrite(LED, HIGH); // Ligar o LED
        sendCommand(" acionar##" + String(delayOption));

      

    } else {
        digitalWrite(LED, LOW); // Desligar o LED
    }
    // Lidar com as solicitações HTTP dos clientes
    server.handleClient();
    delay(1000); // Aguarda 1 segundo
}

void verifica (int state) {
    if (state == RADIOLIB_ERR_NONE) {
        Serial.println(F("LoRa Initialized!"));
        display.clearBuffer();
        display.drawStr(0, 10, "LoRa Initialized! principal");
        display.sendBuffer();
    } else {
        Serial.print(F("LoRa initialization failed, code: "));
        Serial.println(state);
        display.clearBuffer();
        display.drawStr(0, 10, "LoRa initialization failed, code:" + state);
        display.sendBuffer();
        while (true);
    }

    if (Lora.setFrequency(LORA_FREQ) == RADIOLIB_ERR_INVALID_FREQUENCY ||
        Lora.setSpreadingFactor(LORA_SF) == RADIOLIB_ERR_INVALID_SPREADING_FACTOR ||
        Lora.setCodingRate(LORA_CR) == RADIOLIB_ERR_INVALID_CODING_RATE) {
        Serial.println(F("Invalid LoRa configuration!"));
        while (true);
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
