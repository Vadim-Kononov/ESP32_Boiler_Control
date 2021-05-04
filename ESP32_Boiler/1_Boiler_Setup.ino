void setup()
{
pinMode(LED_RED,    OUTPUT); 
pinMode(LED_GREEN,  OUTPUT); 
pinMode(LED_BLUE,   OUTPUT); 

//Отключение brownout detector
WRITE_PERI_REG(RTC_CNTL_BROWN_OUT_REG, 0);

flag_Setup = true;

#if     (DEBUG == 1)
Serial.begin(115200);
#endif

servodrive.attach(SERVO_PIN);                            //Определение пина управления сервопривода
pinMode(SERVO_VCC, OUTPUT);                              //Определение пина питания сервопривода
digitalWrite(SERVO_VCC, HIGH);                           //Отключение питания сервопривода

ds18b20.begin();                                         //Инициация датчиков DS18B20
ds18b20.setResolution(9);                                //Минимальное разрешение

/*Инициализация NVS памяти*/
memory.begin("memory", false);
/*Удаление лишних ключей c именем name*/
//memory.remove("name");
/*Удаление всех ключей*/
//memory.clear();

/*Первоначальное (первое для платы) сохранение логина и пароля WiFi закомментировано*/
//memory.putString("login", "One"); memory.putString("pass", "73737373");

/*Назначение функции WiFiEvent*/
WiFi.onEvent(WiFiEvent);

/*Подключение к WiFi*/
WiFiConnect();

/*Инициализация  ESPNOW*/
if (esp_now_init() != ESP_OK) {DEBUG_PRINTLN(String(F("Error initializing ESP-NOW"))); return;}

/*Регистрация функции обратного вызова при отправке данных ESPNOW*/
esp_now_register_send_cb(Sending_Complete_Now);

/*Регистрация функции обратного вызова при приеме данных ESPNOW*/
esp_now_register_recv_cb(Receiv_Now);

/*Запись информации о пире Slave_1*/
memcpy(peerInfo.peer_addr, mac_Slave_1, 6); peerInfo.channel = 0; peerInfo.encrypt = false;
/*Добавление Slave_1 в список*/
if (esp_now_add_peer(&peerInfo) != ESP_OK) {DEBUG_PRINTLN(String(F("Failed to add peer"))); return;}
/*Запись информации о пире Slave_2*/
memcpy(peerInfo.peer_addr, mac_Slave_2, 6); peerInfo.channel = 0; peerInfo.encrypt = false;
/*Добавление  Slave_2 в список*/
if (esp_now_add_peer(&peerInfo) != ESP_OK) {DEBUG_PRINTLN(String(F("Failed to add peer"))); return;}

/*Инициализация Bluetooth*/
SerialBT.begin(board_name[ROLE]);
SerialBT.register_callback(BT_Event);

/*Инициализация Telnet*/
TelnetServer.begin();
TelnetServer.setNoDelay(true);

/*Назначение функций и параметров OTA*/
ArduinoOTA.setHostname(board_name[ROLE]);
ArduinoOTA.onStart(OTA_Start);

/*Назначение функций и параметров MQTT*/
mqttClient.onConnect(mqtt_Connected_Complete);
mqttClient.onDisconnect(mqtt_Disconnect_Complete);
mqttClient.onSubscribe(mqtt_Subscribe_Complete);
mqttClient.onUnsubscribe(mqtt_Unsubscribe_Complete);
mqttClient.onMessage(mqtt_Receiving_Complete);
mqttClient.onPublish(mqtt_Publishe_Complete);
mqttClient.setServer(mqtt_host, mqtt_port);
mqttClient.setCredentials(mqtt_username, mqtt_pass);
mqttClient.setClientId(board_name[ROLE]);

/*Запись номера платы в структуру*/
now_Store[ROLE].unit = ROLE;

string_position = "⚪ ";                       //Начальная строка для IFTTT сообщения
string_extreme = "";                          //Начальная строка для IFTTT сообщения
string_situation = " ⚪";                     //Начальная строка для отправки в панель MQTT

position_old      = memory.getInt("position");          //Восстановление из памяти позиции регулятора
position_int      = position_old;                       //Начальное значение позиции регулятора
control_mode      = memory.getInt("control_mode", 1);   //Восстановление из памяти режима регулирования
time_min_control  = memory.getInt("time_control", 30);  //Восстановление из памяти периода регулирования
temp_zad          = memory.getFloat("temp_zad", 24.0);  //Восстановление из памяти заданной температуры
hour_down         = memory.getInt("hour_down", 9);      //Восстановления часа снижения заданной температуры
hour_up           = memory.getInt("hour_up", 21);       //Восстановления часа повышения заданной температуры

/*Инкремент счетчика перезагрузок*/
memory.putInt("countReset", memory.getInt("countReset") + 1);

/*Запуск таймера локальных часов*/
xTimerStart(timerClock, 10);
/*Запуск сторожевого таймера*/
xTimerStart(timerWatchDog, 10);
}
