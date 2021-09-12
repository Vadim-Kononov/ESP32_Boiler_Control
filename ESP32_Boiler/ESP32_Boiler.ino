/*
Блок управления энергонезависимым котлом АТЕМ с газовым регулятором Sit630 c использованием сервопривода
Main_0  Плата управления котлом                 (скетч Boiler)
Slave_1 Плата измерения температуры в комнате   (скетч Room)
Slave_2 Плата измерения температуры во дворе    (скетч Outdoor)
*/

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

//Тип платы Main_0, Slave_1, Slave_2 (0, 1, 2). Количество плат = 3
#define ROLE        0
#define BOARD_COUNT 3

//Включение вывода в Serial и (или) Telnet, Bluetooth для отладки
#define DEBUG 0
#if    (DEBUG == 1)
#define DEBUG_PRINT(x)   Serial.print(x)
#define DEBUG_PRINTLN(x) Serial.println(x)
#define DEBUG_WRITE(x, y)   Write(x, y)
#define DEBUG_WRITELN(x, y) Writeln(x, y)
#else
#define DEBUG_PRINT(x)
#define DEBUG_PRINTLN(x)
#define DEBUG_WRITE(x, y)
#define DEBUG_WRITELN(x, y)
#endif
//Включение вывода в Serial и (или) Telnet, Bluetooth для отладки

//Библиотека для отключения brownout detector
#include "soc/rtc_cntl_reg.h"
//Библиотека для получения кодов перезагрузки
#include <rom/rtc.h>
//Файл с данными аккаунтов
#include "Account.h"
//Библиотеки для ESPNOW, OTA, Ping, BT, MQTT, NVS
#include <esp_now.h>
#include <WiFi.h>
#include <ESPmDNS.h>
#include <WiFiUdp.h>
#include <ArduinoOTA.h>
#include <ESP32Ping.h>
#include "BluetoothSerial.h"
#include <AsyncMqttClient.h>
#include <Preferences.h>
#include "time.h"
//Библиотеки для DS18B20, Сервопривода
#include <OneWire.h>
#include <DallasTemperature.h>
#include <Servo.h>

/*Номера GPIO для Сервопривода, DS18b20, RGB светодиода*/
//Servo красный +7,5В, черный -7,5В, белый сигнал.
#define SERVO_PIN   16
#define SERVO_VCC   17 
#define DS18B20     23
#define LED_RED     26
#define LED_GREEN   18
#define LED_BLUE    19


/*Таймеры для Now*/
#define Reset_time  90000 //Время до сброса WDT и OTA таймерами
#define TACT_time   400   //Время смещения выполнения функций отправки данных

//Таймер локальных часов, 1 сек.
TimerHandle_t timerClock      = xTimerCreate("timerClock",      pdMS_TO_TICKS(1000),              pdTRUE,   (void*)0,   reinterpret_cast<TimerCallbackFunction_t>(Clock));
//Сторожевой таймер, Reset_time
TimerHandle_t timerWatchDog   = xTimerCreate("timerWatchDog",   pdMS_TO_TICKS(Reset_time),        pdTRUE,   (void*)0,   reinterpret_cast<TimerCallbackFunction_t>(WatchDog));

//Таймер сброса при неудачном OTA, Reset_time
TimerHandle_t timerOTA        = xTimerCreate("timerOTA",        pdMS_TO_TICKS(Reset_time),        pdFALSE,  (void*)0,   reinterpret_cast<TimerCallbackFunction_t>(OTAReset));
//Таймер отправки Now сообщения, TACT_time
TimerHandle_t timerSendNow    = xTimerCreate("timerSendNow",    pdMS_TO_TICKS(TACT_time),         pdFALSE,  (void*)0,   reinterpret_cast<TimerCallbackFunction_t>(Send_Now));
//Таймер ожидания ответа ThingSpeak сервера, TACT_time
TimerHandle_t timerThSpTime   = xTimerCreate("timerThSpTime",   pdMS_TO_TICKS(TACT_time),         pdFALSE,  (void*)0,   reinterpret_cast<TimerCallbackFunction_t>(ThSpTime));
//Таймер ожидания ответа IFTTTT сервера, TACT_time
TimerHandle_t timerIFTTTTime  = xTimerCreate("timerIFTTTTime",  pdMS_TO_TICKS(TACT_time),         pdFALSE,  (void*)0,   reinterpret_cast<TimerCallbackFunction_t>(IFTTTTime));

/*Таймеры для Boiler*/
//Таймер включения питания Servo после выставления величины поворота
TimerHandle_t timerServoOn    = xTimerCreate("timerServoOn",    pdMS_TO_TICKS(500),               pdFALSE, (void*)0, reinterpret_cast<TimerCallbackFunction_t>(ServoOn));
//Таймер отключения питания Servo после совершенного поворота
TimerHandle_t timerServoOff   = xTimerCreate("timerServoOff",   pdMS_TO_TICKS(5000),              pdFALSE, (void*)0, reinterpret_cast<TimerCallbackFunction_t>(ServoOff));

//Таймеры включения светодиодов на короткое время
TimerHandle_t timerRed        = xTimerCreate("timerRed_Off",    pdMS_TO_TICKS(100),               pdFALSE,   (void*)0,  reinterpret_cast<TimerCallbackFunction_t>(Red_Off));
TimerHandle_t timerGreen      = xTimerCreate("timerGreen_Off",  pdMS_TO_TICKS(100),               pdFALSE,   (void*)0,  reinterpret_cast<TimerCallbackFunction_t>(Green_Off));
TimerHandle_t timerBlue       = xTimerCreate("timerBlue_Off",   pdMS_TO_TICKS(100),               pdFALSE,   (void*)0,  reinterpret_cast<TimerCallbackFunction_t>(Blue_Off));




// Глобальные переменные Now
//Символьные массивы для хранения логина и пароля WiFi в NVS
char ssid [30];
char password [30];

//Имена плат
const char *  board_name[BOARD_COUNT] PROGMEM = {"Boiler", "Room", "Outdoor"};

//MAC адрес Main платы
uint8_t mac_Main_0[] =  {0xA4, 0xCF, 0x12, 0x24, 0xB4, 0x68};
//MAC адреса Slave плат
uint8_t mac_Slave_1[] = {0x24, 0x62, 0xAB, 0xD4, 0xD7, 0x54};
uint8_t mac_Slave_2[] = {0x24, 0x62, 0xAB, 0xD4, 0xCA, 0xF0};
//или широковещательный MAC адрес
//uint8_t broadcastAddress[] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};

//Определение для Bluetooth
BluetoothSerial SerialBT;

//Определения для Telnet
WiFiServer TelnetServer(59990);
WiFiClient TelnetClient;

//Определение для MQTT
AsyncMqttClient mqttClient;

//Определение для NVS памяти
Preferences memory;

//Определения для NTP получения времени
struct tm timeinfo;

volatile uint8_t
seconds = 0, //Секунды текущего времени
minutes = 0, //Минуты текущего времени
hours   = 0, //Часы текущего времени
days    = 0, //Номер дня текущей даты
months  = 0; //Номер месяца текущей даты

volatile uint16_t
min_in_day = 0;                                           //Количество минут с начала суток

bool
flag_Terminal_pass = false,                               //Флаг введенного в Terminal пароля
flag_MQTT_connected = false,                              //Флаг подключения к MQTT серверу
flag_WiFi_connected = false,                              //Флаг подключения к WiFi
flag_Now_connected[BOARD_COUNT] = {false, false, false},  //Флаги получения ответов от Now плат
flag_OTA_begin = false,                                   //Флаг начала OTA загрузки скетча
flag_ThSp_time = false,                                   //Флаг окончания выдержки времени ожидания ответа от ThingSpeak сервера
flag_IFTTT_time = false,                                  //Флаг окончания выдержки времени ожидания ответа от IFTTT сервера
flag_Setup = false;                                       //Флаг произошедшей перезагрузки

volatile bool
flag_seconds = false,                                     //Флаг считывания секунд
flag_minutes = false,                                     //Флаг считывания минут
flag_hours = false;                                       //Флаг считывания часов

/*Cтруктура данных для передачи через ESPNOW*/
struct now_data
{
byte       unit;                  //Номер платы
int        hash;                  //Случайное число для проверки связи
bool       flag_Now_connected_1;  //Флаг связи с устройством 1
bool       flag_Now_connected_2;  //Флаг связи с устройством 2
int        reg_position;          //Положение регулятора
float      temp_gas;              //Температура отходящих газов
float      temp_water;            //Температура воды в котле
float      temp_room;             //Температура в комнате
float      temp_set;              //Заданная температура в комнате
float      hum_room;              //Влажность в комнате
float      press_room;            //Давление в комнате
float      temp_outdoor;          //Температура во дворе
float      hum_outdoor;           //Влажность во дворе
float      press_outdoor;         //Давление во дворе
};

now_data now_Get;                 //Cтруктура данных для приема
now_data now_Store[BOARD_COUNT];  //Массив структур данных для передачи и хранения для каждой платы
esp_now_peer_info_t peerInfo;     //Cтруктура данных для записи информации о пирах ESPNOW
int back_hash;                    //Хэш код для Slave плат

// Глобальные переменные Boiler
OneWire onewire(DS18B20);                                                         //Определение для DS18B20
DallasTemperature ds18b20(&onewire);                                              //Определение для DS18B20
DeviceAddress ds18b20_water =  {0x28, 0xF0, 0x76, 0x79, 0xA2, 0x00, 0x03, 0xFD};  //Адрес датчика тепературы воды
DeviceAddress ds18b20_gas =    {0x28, 0xFF, 0x9D, 0x0E, 0xA6, 0x16, 0x04, 0x06};  //Адрес датчика тепературы отходящих газов
Servo servodrive;                                                                 // Определение для сервопривода

bool
flag_temp_room_old = true,   //Флаг отсутсвия значения старой тепературы после перезагрузки
flag_notice        = true,   //Флаг разрешения отправки IFTTT уведомлений о выполнении регулировки
flag_position      = false;  //Флаг выполнения поворота Servo в настоящий момент 

volatile bool
flag_time_control  = false;  //Флаг наступления времени выполнения регулировки

int
position_int,                 //Положение регулятора
position_old,                 //Предыдущее положение регулятора
turn_counter,                 //Счетчик поворотов
control_mode,                 //Режим работы регулятора 0 - ручной, 1 автоматический
time_min_control;             //Время в минутах между съемом показаний и выполнением регулировки

int8_t
position_dif,                 //Изменение в положении регулятора
hour_down,                    //Час снижения заданной температуры
hour_up;                      //Час повышения заданной температуры

long
time_msec_control,            //Время в msec прошедшее с последней регулировки
time_msec_position;           //Время в msec прошедшее с последнего поворота регулятора

float
temp_room,                                                      //Температура в комнате
temp_room_old,                                                  //Температура в комнате, предшествующая
temp_room_saved [8],                                            //Температуры в комнате, предшествующие, сохраненные для графика
temp_zad,                                                       //Температура в комнате, заданная
temp_dif,                                                       //Температура в комнате, разность с заданной
temp_trend,                                                     //Температура в комнате, прогноз изменения за цикл регулирования
temp_gas_old,                                                   //Температура отходящих газов предшествующая
position_dec;                                                   //Положение Servo в виде десятичного числа

String 
string_temp_room,             //Строка содержащая либо числовое значение температуры, либо символ отсутсвия связи
string_temp_dif,              //Строка содержащая либо числовое значение температуры, либо символ отсутсвия связи
string_temp_trend,            //Строка содержащая либо числовое значение температуры, либо символ отсутсвия связи
string_position,              //Строка содержащая символ направления изменения позиции регулятора
string_gas_state,             //Строка содержащая состояние горелки котла
string_mode_state,            //Строка содержащая состояние режима регулировки температуры
string_situation,             //Строка содержащая символ направления ожидаемого изменения позиции регулятора
string_extreme,               //Строка содержащая символ достигнутого крайнего верхнего и нижнего положений регулятора
string_hours,                 //Строка содержащая текущий час
string_minutes,               //Строка содержащая текущую минуту
string_seconds,               //Строка содержащая текущую секунду
string_status_1,              //Строка состояния 1
string_status_2;              //Строка состояния 2
