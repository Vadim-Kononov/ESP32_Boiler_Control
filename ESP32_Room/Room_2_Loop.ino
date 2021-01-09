void loop()
{
ArduinoOTA.handle();

/*Получение строки из Bluetooth, при ее наличии. Отправка строки в Terminal для обработки*/
if (SerialBT.available()) Terminal(SerialBT.readStringUntil('\n'), B100);

/*Подключения клиента Telnet*/
if (TelnetServer.hasClient())
{
if(TelnetClient) TelnetClient.stop(); TelnetClient = TelnetServer.available();
TelnetClient.println("> \"" + String (board_name[ROLE]) + "\"");
TelnetClient.println("> IP: " + WiFi.localIP().toString() + "  |  MAC: " +  WiFi.macAddress());
TelnetClient.println("> CPU0: " + StringResetReason(rtc_get_reset_reason(0)));
TelnetClient.println("> CPU1: " + StringResetReason(rtc_get_reset_reason(1)));
}

/*Получение строки из Telnet, при ее наличии. Отправка строки в Terminal для обработки*/
if (TelnetClient && TelnetClient.connected() && TelnetClient.available()) {while(TelnetClient.available()) Terminal(TelnetClient.readStringUntil('\n'), B010);}

if (flag_OTA_begin) return;
/*При начале OTA все, что ниже не выполняется*/


/*Cycle_A, длительные операции*/
if (flag_Cycle_A)
{
flag_Cycle_A = false;
/*Сброс сторожевого таймера*/
xTimerReset(timerWatchDog, 0);

//Очистка экрана через 60 циклов
cycle_count --;
if (cycle_count <= 0) {cycle_count = 60; tft.fillScreen(background_color); tft.drawRect(0, 0, 240, 240, WHITE);}

//Вывод прямых и окружности
flag_Now_connected[0] ? temp_color = WHITE : temp_color = RED;
tft.drawLine(20, 88, 220, 88, temp_color);
tft.drawLine(20, 152, 220, 152, temp_color);
tft.drawCircle(120, 120, 28, temp_color);

//Вывод положения регулятора
switch (now_Store[0].reg_position)
{
  case 1:
    temp_color = BLUE;
    break;
  case 2:
    temp_color = DARKGREEN;
    break;
  case 3:
  case 4:
    temp_color = YELLOW;
    break;
  default:
    temp_color = RED;
    break;
}

flag_Now_connected[0] ? text_color = temp_color : text_color = background_color; tft.setTextColor(text_color, background_color);
tft.setTextSize(4);
tft.setCursor(110, 106);
tft.print(now_Store[0].reg_position);

//Вывод температуры воды
flag_Now_connected[0] ? text_color = PINK : text_color = background_color; tft.setTextColor(text_color, background_color);
tft.setTextSize(3);
tft.setCursor(20, 108);
tft.print(now_Store[0].temp_water, 0);

//Вывод температуры газа
flag_Now_connected[0] ? text_color = PINK : text_color = background_color; tft.setTextColor(text_color, background_color);
tft.setTextSize(3);
tft.setCursor(184, 108);
tft.print(now_Store[0].temp_gas, 0);

flag_Now_connected[0] && now_Store[0].flag_Now_connected_2 ? text_color = CYAN : text_color = background_color; tft.setTextColor(text_color, background_color);
tft.setTextSize(5);

//Вывод температуры на улице
String str;

//abs(now_Store[0].temp_outdoor)>=10.0 ? str = String(now_Store[0].temp_outdoor, 1) : str = String(now_Store[0].temp_outdoor, 2);
str = String(now_Store[0].temp_outdoor, 1);
tft.setCursor((240-30*str.length())/2, 161);
tft.fillRect(10, 159, 220, 44, background_color);
tft.print(str);


tft.setTextSize(3);

//Вывод давления на улице
tft.setCursor(12, 210);
tft.fillRect(10, 208, 112, 28, background_color);
tft.print(now_Store[0].press_outdoor, 1);

//Вывод влажности на улице
tft.setCursor(162, 210);
tft.fillRect(160, 208, 76, 28, background_color);
tft.print(now_Store[0].hum_outdoor, 1);

BME280::TempUnit tempUnit(BME280::TempUnit_Celsius);
BME280::PresUnit presUnit(BME280::PresUnit_Pa);

bme.read(now_Store[ROLE].press_room, now_Store[ROLE].temp_room, now_Store[ROLE].hum_room, tempUnit, presUnit);   
                               
now_Store[ROLE].temp_room = now_Store[ROLE].temp_room + temp_correction;                                    //Коррекция температуры
now_Store[ROLE].press_room = now_Store[ROLE].press_room * 0.00750063755419211 + press_correction;           //Коррекция давления
now_Store[ROLE].hum_room = now_Store[ROLE].hum_room + hum_correction;                                       //Коррекция влажности



tft.setTextColor(GREEN, background_color);
tft.setTextSize(5);

//Вывод температуры в комнате
tft.setCursor(65, 41);
tft.print(now_Store[ROLE].temp_room,1);

tft.setTextSize(3);

//Вывод давления в комнате
tft.setCursor(12, 7);
tft.print(now_Store[ROLE].press_room,1);

//Вывод влажности в комнате
tft.setCursor(162, 7);
tft.print(now_Store[ROLE].hum_room,1);





}
/**/



/*Cycle_B, длительные операции*/
if (flag_Cycle_B)
{
flag_Cycle_B = false;
WiFiReconnect ();
}
/**/

//-------------------------------------------------------------------------------------------------------------



}
