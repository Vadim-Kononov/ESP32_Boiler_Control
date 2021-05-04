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

/*Повторяется каждый цикл Loop*/
/*Повторяется каждую не четную секунду*/
if (seconds % 2 == 1 && flag_seconds)
{
flag_seconds = false;
//Мигаем красным светодиодом
digitalWrite(LED_RED, HIGH); xTimerStart(timerRed, 10); 
//Запускаем отправку Now данных 
Cycle_A ();
//Запускаем измерене температуры воды датчиком DS18B20  
ds18b20.requestTemperatures(); 
//Если датчик не отвечаем оставляем старое значение 
if (ds18b20.isConnected(ds18b20_water))  now_Store[ROLE].temp_water  = ds18b20.getTempC(ds18b20_water);
if (ds18b20.isConnected(ds18b20_gas))    now_Store[ROLE].temp_gas    = ds18b20.getTempC(ds18b20_gas);

if        (now_Store[ROLE].temp_gas > temp_gas_old && now_Store[ROLE].temp_gas >= 40.0) string_gas_state = "🔥";
else if   (now_Store[ROLE].temp_gas < temp_gas_old || now_Store[ROLE].temp_gas <  40.0) string_gas_state = "☁";
temp_gas_old = now_Store[ROLE].temp_gas;

switch (control_mode)
  {
  case 0:
    string_mode_state = "✋";
    break;
  case 1:
    string_mode_state = "🅰";
    break;
  case 2:
    if (hours >= hour_down && hours <= hour_up) string_mode_state = String ("🅰 ") + String(hour_up) + "↑"; 
    else string_mode_state = String ("🅰 ") + String(hour_down) + "↓"; 
    break;
  case 3:
    if (hours >= hour_down && hours <= hour_up) string_mode_state = String ("🅰 ") + String(hour_up) + "↑"; 
    else string_mode_state = String ("🅰 ") + String(hour_down) + "↓"; 
    break;
  }
}
/**/



/*Повторяется каждую четную секунду*/
if (seconds % 2 == 0 && flag_seconds)
{  flag_seconds = false;
           
   min_in_day = 60 * hours + minutes;
   if (hours >= 10) string_hours = String (hours); else string_hours = "0" + String (hours);
   if (minutes >= 10) string_minutes = String (minutes); else string_minutes = "0" + String (minutes);
   if (seconds >= 10) string_seconds = String (seconds); else string_seconds = "0" + String (seconds);
   
   if (min_in_day % time_min_control == 0 && flag_time_control)                 //Если подошло время работы регулятора
   {       
      flag_time_control = false;                                                //Сбрасываеи флаг работы регулятора для исключения ненужных повторений
      time_msec_control = xTaskGetTickCount();                                  //Обнуляем счетчик миллисекунд времени от начала регулирования   
      temp_room_old = now_Store[ROLE].temp_room;                                //Сохранение значения температуры
      
      temp_room_saved [7] = temp_room_saved [6];
      temp_room_saved [6] = temp_room_saved [5];
      temp_room_saved [5] = temp_room_saved [4];
      temp_room_saved [4] = temp_room_saved [3];
      temp_room_saved [3] = temp_room_saved [2];
      temp_room_saved [2] = temp_room_saved [1];
      temp_room_saved [1] = temp_room_saved [0];
      temp_room_saved [0] = temp_room_old;
          
              if (position_int + Situation() > 21) {position_int = 21; string_extreme = " ➕";} //Вызываем Situation() и проверяем крайность в + позиции регулятора
        else  if (position_int + Situation() < 1)  {position_int = 1; string_extreme = " ➖";}  //Вызываем Situation() и проверяем крайность в - позиции регулятора
        else     {position_int = position_int + Situation(); string_extreme = "";}              //Вызываем Situation() и определяем отсутсвие крайности позиции регулятора
        Position (position_int);                                                                //Вызываем функцию поворота 
                
   }          
  mqtt_Send ();                        //Отправка MQTT сообщений
}
/**/


/*Повторяется каждую минуту*/
if (flag_minutes)
{
  flag_minutes = false;
  WiFiReconnect ();                    //Переподключение к WiFi при обрыве связи
  randomSeed(micros());                //Изменение последовательности случайных чисел
  ThingSpeakSend ();                   //Отправка IFTTT сообщений
}
/**/


/*Повторяется каждый час*/
if (flag_hours)
{
flag_hours = false;

if (hours == hour_down || hours == hour_up)                         
{
  switch (control_mode)
  {
  case 2:
    if      (hours == hour_down) temp_zad = temp_zad - 0.5;
    else if (hours == hour_up)   temp_zad = temp_zad + 0.5;
    break;
  case 3:
    if      (hours == hour_down) temp_zad = temp_zad - 1.0;
    else if (hours == hour_up)   temp_zad = temp_zad + 1.0;
    break;
  }
  memory.putFloat("temp_zad", temp_zad);
  if (flag_notice) IFTTTSend (String(ifttt_event), String(board_name[ROLE]) + " " + String(F("Set temperature")), String(temp_zad, 1), "℃");
}

}
/**/



/*Сброс сторожевого таймера*/
xTimerChangePeriod(timerWatchDog,  pdMS_TO_TICKS(Reset_time), 0);  
}
