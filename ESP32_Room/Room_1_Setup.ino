void setup()
{
flag_Setup = true;

#if   (DEBUG == 1)
Serial.begin(115200);
#endif

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

/*Запись информации о пире Main*/
memcpy(peerInfo.peer_addr, mac_Main_0, 6); peerInfo.channel = 0; peerInfo.encrypt = false;
/*Добавление пира Main в список*/
if (esp_now_add_peer(&peerInfo) != ESP_OK) {DEBUG_PRINTLN(String(F("Failed to add peer"))); return;}

/*Инициализация Bluetooth*/
SerialBT.begin(board_name[ROLE]);

/*Инициализация Telnet*/
TelnetServer.begin();
TelnetServer.setNoDelay(true);

/*Назначение функций и параметров OTA*/
ArduinoOTA.setHostname(board_name[ROLE]);
ArduinoOTA.onStart(OTA_Start);

/*Запись номера платы в структуру*/
now_Store[ROLE].unit = ROLE;

/*Инициализация дисплея*/
SPI.begin(TFT_SCLK, TFT_MISO, TFT_MOSI);
tft.init(240, 240, SPI_MODE3);  
tft.setRotation(2);
tft.fillScreen(background_color);
tft.setTextColor(WHITE, background_color);
tft.setTextWrap(1);
tft.drawRect(0, 0, 240, 240, WHITE);

/*Инициализация BME280*/
Wire.begin();
bme.begin();

/*Загрузка коррекции*/
temp_correction     = memory.getFloat("correction_t", 0.0);
press_correction    = memory.getFloat("correction_p", 0.0);
hum_correction      = memory.getFloat("correction_h", 0.0);

/*Инкремент счетчика перезагрузок*/
memory.putInt("countReset", memory.getInt("countReset") + 1);

/*Запуск таймера малого цикла*/
xTimerStart(timerCycle_A, 0);
/*Запуск таймера большого цикла*/
xTimerStart(timerCycle_B, 0);
/*Запуск сторожевого таймера*/
xTimerStart(timerWatchDog, 0);
}
