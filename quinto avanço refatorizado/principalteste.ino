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

static const char *WIFI_SSID = "";
static const char *WIFI_PASSWORD = "";
static const uint8_t STATUS_LED_PIN = 2;
static const char *CMD_ACTIVATE_PREFIX = "A";
static const char *CMD_HUMIDITY_PREFIX = "H:";
static const char *LEGACY_CMD_HUMIDITY_PREFIX = "umidade:";
static const unsigned long LOOP_DELAY_MS = 20;
static const int LORA_TX_POWER_DBM = 14;


const char *ntpServer = "pool.ntp.org";
const long gmtOffset_sec = -3 * 3600;
const int daylightOffset_sec = 0;

WebServer server(80);


// Definição dos pinos SPI para o módulo LoRa
#define LORA_CLK   SCK     // Pino de clock SPI
#define LORA_MISO  MISO    // Pino MISO SPI
#define LORA_MOSI  MOSI    // Pino MOSI SPI
#define LORA_NSS   SS      // Pino de seleção de escravo SPI (Chip Select)


#define LORA_FREQ  915.0   // Frequência do LoRa em MHz
#define LORA_SF    7       // Fator de espalhamento do LoRa
#define LORA_CR    5       // Taxa de codificação do LoRa

U8G2_SSD1306_128X64_NONAME_F_SW_I2C display(U8G2_R0, 18,17,21);

SX1262 Lora = new Module(8,14,12,13);

volatile bool transmitFlag = false;
volatile bool permitirInterrupcao = true;

void verifica (int state);
void sendCommand(String command);
void connectWiFi();
bool isHumidityPacket(const String &payload);
int parseHumidity(const String &payload);
void restartReceive();

unsigned long lastNtpUpdate = 0;
const unsigned long ntpUpdateInterval = 600000;


WiFiUDP ntpUDP;
NTPClient timeClient(ntpUDP, ntpServer, gmtOffset_sec, daylightOffset_sec);

int m=0;
int h=0;
int delayOption = 1; 
bool commandSentForCurrentMinute = false;
bool scheduleConfigured = false;

void handleRoot() {
    server.send(200, "text/html", "<h1>Definir Hora e Delay</h1><form action='/setTime' method='post'>Hora: <input type='text' name='hour'><br>Minuto: <input type='text' name='minute'><br>Delay: <select name='delay'><option value='1'>30 segundos</option><option value='2'>60 segundos</option><option value='3'>90 segundos</option></select><br><input type='submit' value='Definir'></form>");
}

void handleSetTime() {
    String hour = server.arg("hour");
    String minute = server.arg("minute");
    String delay = server.arg("delay");

    h = hour.toInt();
    m = minute.toInt();
    delayOption = delay.toInt();
    commandSentForCurrentMinute = false;
    scheduleConfigured = true;

    Serial.print("Hora definida: ");
    Serial.print(h);
    Serial.print(":");
    Serial.println(m);

    Serial.print("Delay definido: ");
    Serial.println(delayOption);

    server.send(200, "text/plain", "Hora e delay definidos com sucesso!");
}

void setup() {
    Serial.begin(115200);
    display.begin();
    display.setFont(u8g2_font_NokiaSmallPlain_te );

    pinMode(STATUS_LED_PIN, OUTPUT);
    digitalWrite(STATUS_LED_PIN, LOW);

    SPI.begin(LORA_CLK, LORA_MISO, LORA_MOSI, LORA_NSS);

    int state = Lora.begin();
    verifica(state);

    connectWiFi();

    timeClient.begin();
    timeClient.update();

    server.on("/", HTTP_GET, handleRoot);
    server.on("/setTime", HTTP_POST, handleSetTime);
    server.begin();


    Lora.setDio1Action(setFlag);

    state = Lora.startReceive();
    if (state != RADIOLIB_ERR_NONE) {
        Serial.print(F("Failed to start LoRa reception, code: "));
        Serial.println(state);
        while (true);
    }
}

void setFlag(void) {
    if (!permitirInterrupcao) {
        return;
    }
    transmitFlag = true;
}

void loop() {
  

    
    unsigned long currentMillis = millis();

    if (currentMillis - lastNtpUpdate >= ntpUpdateInterval) {
        timeClient.update();
        lastNtpUpdate = currentMillis;
    }

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
        permitirInterrupcao = false;
        Serial.println(F("Receiving finished: "));
        String str;
        int state = Lora.readData(str);

        if (state == RADIOLIB_ERR_NONE) {
            Serial.print(F("Received data successfully!"));
            Serial.println(str);

            if (isHumidityPacket(str)) {
                int umidade = parseHumidity(str);
                digitalWrite(STATUS_LED_PIN, umidade > 50 ? HIGH : LOW);
            }

            int16_t rssi = Lora.getRSSI();
            int16_t rssiDetection = abs(rssi);

            display.clearBuffer();
            display.drawStr(0, 10, "LoRa Data:");
            display.drawStr(0, 20, (String("Data: ") + str).c_str());
            display.drawStr(0, 30, (String("Size: ") + String(str.length()) + " RSSI: " + String(rssiDetection)).c_str());
            display.sendBuffer();
        } else {
            Serial.print(F("Failed to receive data, code: "));
            Serial.println(state);
        }

        restartReceive();
        permitirInterrupcao = true;
    } else {
        delay(LOOP_DELAY_MS);
    }

    if (scheduleConfigured && currentHour == h && currentMinute == m) {
        if (!commandSentForCurrentMinute) {
            digitalWrite(STATUS_LED_PIN, HIGH);
            sendCommand(String(CMD_ACTIVATE_PREFIX) + String(delayOption));
            commandSentForCurrentMinute = true;
        }
    } else {
        commandSentForCurrentMinute = false;
        digitalWrite(STATUS_LED_PIN, LOW);
    }
    server.handleClient();
    delay(LOOP_DELAY_MS);
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
        display.drawStr(0, 10, (String("LoRa initialization failed, code: ") + state).c_str());
        display.sendBuffer();
        while (true);
    }

    if (Lora.setFrequency(LORA_FREQ) == RADIOLIB_ERR_INVALID_FREQUENCY ||
        Lora.setSpreadingFactor(LORA_SF) == RADIOLIB_ERR_INVALID_SPREADING_FACTOR ||
        Lora.setCodingRate(LORA_CR) == RADIOLIB_ERR_INVALID_CODING_RATE ||
        Lora.setOutputPower(LORA_TX_POWER_DBM) == RADIOLIB_ERR_INVALID_OUTPUT_POWER) {
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
    restartReceive();
}

void connectWiFi() {
    if (String(WIFI_SSID).length() == 0) {
        WiFi.mode(WIFI_AP);
        WiFi.softAP("ESP32-LoRa");

        Serial.println("WiFi nao configurado, iniciando AP");
        Serial.println(WiFi.softAPIP());

        display.clearBuffer();
        display.drawStr(0, 10, "Modo AP ativo");
        display.drawStr(0, 20, "ESP32-LoRa");
        display.sendBuffer();
        return;
    }

    WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
    Serial.print("Conectando ao WiFi");

    while (WiFi.status() != WL_CONNECTED) {
        delay(500);
        Serial.print(".");
    }

    Serial.println();
    Serial.println("Conectado ao WiFi com sucesso");
    Serial.println(WiFi.localIP());

    display.clearBuffer();
    display.drawStr(0, 10, "WiFi conectado");
    display.drawStr(0, 20, String(WIFI_SSID).c_str());
    display.sendBuffer();
}

bool isHumidityPacket(const String &payload) {
    return payload.startsWith(CMD_HUMIDITY_PREFIX) || payload.startsWith(LEGACY_CMD_HUMIDITY_PREFIX);
}

int parseHumidity(const String &payload) {
    String value = payload;

    if (payload.startsWith(CMD_HUMIDITY_PREFIX)) {
        value = payload.substring(String(CMD_HUMIDITY_PREFIX).length());
    } else if (payload.startsWith(LEGACY_CMD_HUMIDITY_PREFIX)) {
        value = payload.substring(String(LEGACY_CMD_HUMIDITY_PREFIX).length());
    }

    value.replace("%", "");
    value.trim();
    return value.toInt();
}

void restartReceive() {
    int state = Lora.startReceive();
    if (state != RADIOLIB_ERR_NONE) {
        Serial.print(F("Failed to restart LoRa reception, code: "));
        Serial.println(state);
    }
}
