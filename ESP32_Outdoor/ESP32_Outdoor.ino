/*
ESP32_Exchange_Multiple шаблон для ESP32
Oбмен между несколькими, в данном скетче между тремя ESP32 без WiFi и роутера по технологии ESPNOW с проверкой канала связи 
Обмен по Telnet, обмен по Bluetooth, обмен по MQTT
Вывод данных на ThingSpeak, вывод данных на IFTTT, получение времени по NTP
Сохранение данных в NVS, сторожевой таймер WDT
Загрузка скетча через OTA

ROLE 0  Main_0 плата, иницирует обмен со Slave платами через ESPNOW, осуществляет обмен по MQTT, выводит данные в ThingSpeak, IFTTT
        В цикле таймера Cycle_A отправляет сообщения ESPNOW, MQTT, NTP, WDT
        --> Cycle_A --> Send_Now --TACT_time--> Receiv_Now --TACT_time--> Receiv_Now --TACT_time--> mqtt_Send --> 
        В цикле таймера timerCycle_B отправляет сообщения на ThingSpeak, переподключается к WiFi и MQTT при потере соединения
        WiFi.macAddress A4:CF:12:24:B4:68 (COM4)

ROLE 1  Slave_1 плата, отвечает на сообщения Main_0 платы по ESPNOW после приема сообщения
        Receiv_Now --TACT_time--> Send_Now
        WiFi.macAddress 24:62:ab:d4:d7:54 (COM3)  

ROLE 2  Slave_2 плата, отвечает на сообщения Main_0 платы по ESPNOW после приема сообщения
        Receiv_Now --TACT_time--> --TACT_time--> Send_Now
        WiFi.macAddress 24:62:ab:d4:ca:f0 (COM5)

        Широковещательный MAC адрес для возможных вариантов:
        FF:FF:FF:FF:FF:FF

DEBUG 1 Включает вывод отладочных сообщений в Serial и (или) Telnet, Bluetooth
*/

/*>-----------< Тип платы Main_0, Slave_1, Slave_2 (0, 1, 2). Количество плат = 3>-----------<*/
#define ROLE        2
#define BOARD_COUNT 3
/*>-----------< Тип платы Main_0, Slave_1, Slave_2 (0, 1, 2). Количество плат = 3>-----------<*/

/*Имена плат*/
const char *  board_name[BOARD_COUNT] PROGMEM = {"Boiler", "Room", "Outdoor"};

/*MAC адрес Main платы*/
uint8_t mac_Main_0[] =  {0xA4, 0xCF, 0x12, 0x24, 0xB4, 0x68};
/*MAC адреса Slave плат*/
uint8_t mac_Slave_1[] = {0x24, 0x62, 0xAB, 0xD4, 0xD7, 0x54};
uint8_t mac_Slave_2[] = {0x24, 0x62, 0xAB, 0xD4, 0xCA, 0xF0};
/*или широковещательный MAC адрес*/
//uint8_t broadcastAddress[] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};

/*>-----------< Включение вывода в Serial и (или) Telnet, Bluetooth для отладки >-----------<*/
#define DEBUG 0

#if     (DEBUG == 1)
#define DEBUG_PRINT(x) Serial.print(x)
#define DEBUG_PRINTLN(x) Serial.println(x)
#define DEBUG_WRITE(x, y) Write(x, y)
#define DEBUG_WRITELN(x, y) Writeln(x, y)
#else
#define DEBUG_PRINT(x)
#define DEBUG_PRINTLN(x)
#define DEBUG_WRITE(x, y)
#define DEBUG_WRITELN(x, y)
#endif
/*>-----------< Включение вывода в Serial и (или) Telnet, Bluetooth для отладки >-----------<*/

/*Символьные массивы для хранения логина и пароля WiFi в NVS*/
char ssid [30];
char password [30];

#include "Account.h"

//Библиотека для получения кодов перезагрузки
#include <rom/rtc.h>

/*Флаги введения пароля для Terminal, подключения к MQTT, подключения к WiFi, подтверждение приема Now,
таймера малого цикла, таймера большого цикла, разрешения загрузки по OTA, таймера ожидания ответа ThingSpeak, таймера ожидания ответа IFTTT*/
bool flag_Terminal_pass = false, flag_MQTT_connected = false, flag_WiFi_connected = false, flag_Now_connected[BOARD_COUNT] = {false, false, false},
flag_Cycle_A = false, flag_Cycle_B = false, flag_OTA_begin = false, flag_ThSp_time = false, flag_IFTTT_time = false, flag_Setup = false, flag_send_now = true;

                    

/*Cтруктура данных для передачи через ESPNOW*/
struct now_data
{
byte       unit;                  //Номер платы
int        hash;                  //Случайное число для проверки связи
bool       flag_Now_connected_1;  //Флаг связи с устройством 1
bool       flag_Now_connected_2;  //Флаг связи с устройством 2
int        reg_position;
float      temp_gas;
float      temp_water;
float      temp_room;
float      temp_set;
float      hum_room;
float      press_room;
float      temp_outdoor;
float      hum_outdoor;
float      press_outdoor;
};
/*Cтруктура данных для приема*/
now_data now_Get;
/*Массив структур данных для передачи и хранения для каждой платы*/
now_data now_Store[BOARD_COUNT];
float temp_correction, press_correction, hum_correction;
/*Переменная хранящая старое значение кода проверки для Slave плат*/
int back_hash;

/*Библиотеки для ESPNOW, OTA, Ping*/
#include <esp_now.h>
#include <WiFi.h>
#include <ESPmDNS.h>
#include <WiFiUdp.h>
#include <ArduinoOTA.h>
#include <ESP32Ping.h>

/*Cтруктура данных для записи информации о пирах ESPNOW*/
esp_now_peer_info_t peerInfo;

/*Определения для Bluetooth*/
#include "BluetoothSerial.h"
BluetoothSerial SerialBT;

/*Определения для Telnet*/
WiFiServer TelnetServer(59990);
WiFiClient TelnetClient;

/*Определения для MQTT*/
#include <AsyncMqttClient.h>
AsyncMqttClient mqttClient;

/*Определения для NVS памяти*/
#include <Preferences.h>
Preferences memory;

/*Определения для светодиода*/
#define LED_RED     17
#define LED_GREEN   16
#define LED_BLUE    5

/*Определения для BME280*/
#include <BME280I2C.h>
//Default : forced mode, standby time = 1000 ms, Oversampling = pressure ×1, temperature ×1, humidity ×1, filter off
//Пины по умолчанию GPIO22 Wire SCL, GPIO21 Wire SDA, в связи с этим определения нет
//Измененные параметры
BME280I2C::Settings settings(
   BME280::OSR_X16,
   BME280::OSR_X16,
   BME280::OSR_X16,
   BME280::Mode_Normal,
   BME280::StandbyTime_1000ms,
   BME280::Filter_16,
   BME280::SpiEnable_False
   //BME280I2C::I2CAddr_0x76 // I2C address. I2C specific.
);

BME280I2C bme(settings);
#include <Wire.h>
 
/*Таймеры*/
#define Cycle_A_time        2000                             //Время малого цикла, ESPNOW, MQTT, NTP, WDT
#define Cycle_B_time        60000                            //Время большого цикла, ThingSpeak, переподключение WiFi, MQTT
#define Reset_time          90000                            //Время для таймеров перезагрузки
#define TACT_time           Cycle_A_time/(BOARD_COUNT+2)     //Время смещения выполнения функций выполняющихся в малом цикле
#define Led_time            50

TimerHandle_t timerCycle_A    = xTimerCreate("timerCycle_A",    pdMS_TO_TICKS(Cycle_A_time),      pdTRUE,   (void*)0,   reinterpret_cast<TimerCallbackFunction_t>(Cycle_A));
TimerHandle_t timerCycle_B    = xTimerCreate("timerCycle_B",    pdMS_TO_TICKS(Cycle_B_time),      pdTRUE,   (void*)0,   reinterpret_cast<TimerCallbackFunction_t>(Cycle_B));
TimerHandle_t timerWatchDog   = xTimerCreate("timerWatchDog",   pdMS_TO_TICKS(Reset_time),        pdTRUE,   (void*)0,   reinterpret_cast<TimerCallbackFunction_t>(WatchDog));

TimerHandle_t timerRed_Off    = xTimerCreate("timerRed_Off",    pdMS_TO_TICKS(Led_time),          pdFALSE,   (void*)0,  reinterpret_cast<TimerCallbackFunction_t>(Red_Off));
TimerHandle_t timerGreen_Off  = xTimerCreate("timerGreen_Off",  pdMS_TO_TICKS(Led_time),          pdFALSE,   (void*)0,  reinterpret_cast<TimerCallbackFunction_t>(Green_Off));
TimerHandle_t timerBlue_Off   = xTimerCreate("timerBlue_Off",   pdMS_TO_TICKS(Led_time),          pdFALSE,   (void*)0,  reinterpret_cast<TimerCallbackFunction_t>(Blue_Off));

TimerHandle_t timerOTA        = xTimerCreate("timerOTA",        pdMS_TO_TICKS(Reset_time),        pdFALSE,  (void*)0,   reinterpret_cast<TimerCallbackFunction_t>(OTAReset));
TimerHandle_t timerSendNow    = xTimerCreate("timerSendNow",    pdMS_TO_TICKS(TACT_time),         pdFALSE,  (void*)0,   reinterpret_cast<TimerCallbackFunction_t>(Send_Now));
TimerHandle_t timerIFTTTTime  = xTimerCreate("timerIFTTTTime",  pdMS_TO_TICKS(TACT_time),         pdFALSE,  (void*)0,   reinterpret_cast<TimerCallbackFunction_t>(IFTTTTime));

/*>-----------< Функции >-----------<*/

void Red_Off()    {digitalWrite(LED_RED, LOW);}
void Green_Off()  {digitalWrite(LED_GREEN, LOW);}
void Blue_Off()   {digitalWrite(LED_BLUE, LOW);}


/*Сторожевой таймер*/
void WatchDog() {DEBUG_PRINTLN ("Timer WatchDog went off"); ESP.restart();}
/**/

/*Таймер OTA загрузки*/
void OTAReset() {DEBUG_PRINTLN ("Timer OTA went off"); ESP.restart();}
/**/

/*Остановка таймеров перед OTA загрузкой*/
void OTA_Start()
{
flag_OTA_begin = true;
xTimerStop(timerCycle_A,  0);
xTimerStop(timerCycle_B,  0);
xTimerStop(timerWatchDog, 0);
esp_now_deinit();
mqttClient.disconnect();
DEBUG_PRINTLN ("\nStart updating");
xTimerStart(timerOTA, 0);
/**/
}

void BT_Event(esp_spp_cb_event_t event, esp_spp_cb_param_t *param)
{
  if(event == ESP_SPP_SRV_OPEN_EVT)
  {
  SerialBT.println("> \"" + String (board_name[ROLE]) + "\"");
  SerialBT.println("> IP: " + WiFi.localIP().toString() + "  |  MAC: " +  WiFi.macAddress());
  SerialBT.println("> CPU0: " + StringResetReason(rtc_get_reset_reason(0)));
  SerialBT.println("> CPU1: " + StringResetReason(rtc_get_reset_reason(1)));
  }
}


String StringResetReason(RESET_REASON reason)
{
  switch (reason)
  {
    case 1  : return "Vbat power on reset";break;
    case 3  : return "Software reset digital core";break;
    case 4  : return "Legacy watch dog reset digital core";break;
    case 5  : return "Deep Sleep reset digital core";break;
    case 6  : return "Reset by SLC module, reset digital core";break;
    case 7  : return "Timer Group0 Watch dog reset digital core";break;
    case 8  : return "Timer Group1 Watch dog reset digital core";break;
    case 9  : return "RTC Watch dog Reset digital core";break;
    case 10 : return "Instrusion tested to reset CPU";break;
    case 11 : return "Time Group reset CPU";break;
    case 12 : return "Software reset CPU";break;
    case 13 : return "RTC Watch dog Reset CPU";break;
    case 14 : return "for APP CPU, reseted by PRO CPU";break;
    case 15 : return "Reset when the vdd voltage is not stable";break;
    case 16 : return "RTC Watch dog reset digital core and rtc module";break;
    default : return "Unknown";
  }
}

/*>-----------< Функции >-----------<*/
