/*Сторожевой Таймер*/
void WatchDog()
{
DEBUG_PRINTLN ("Timer WatchDog went off"); ESP.restart();
}
/**/

/*OTA загрузка таймер*/
void OTAReset()
{
DEBUG_PRINTLN ("Timer OTA went off"); ESP.restart();
}
/**/

/*OTA загрузка Остановка Таймеров*/
void OTA_Start()
{
flag_OTA_begin = true;
xTimerStop(timerClock, 0);
xTimerStop(timerWatchDog, 0);
esp_now_deinit();
mqttClient.disconnect();
DEBUG_PRINTLN ("\nOTA_begin");
xTimerStart(timerOTA, 0);
}
/**/
 
/*BT произошло подключение*/
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
/**/

/*Reset Причина перезагрузки*/
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
/**/

/*NTP Получение времени*/
void GetTime()
{
if  (flag_WiFi_connected && Ping.ping("8.8.8.8", 1))
{
  configTime(10800, 0, "pool.ntp.org");
  if (getLocalTime(&timeinfo))
  {
    seconds = timeinfo.tm_sec;
    minutes = timeinfo.tm_min;
    hours =   timeinfo.tm_hour;
    days =    timeinfo.tm_mday;
    months =  timeinfo.tm_mon + 1; 
  }
}
}
/**/

/*Локальные часы */
void Clock ()
{
seconds++;                               flag_seconds = true;
if (seconds > 59) {seconds=0; minutes++; flag_minutes = true; flag_time_control = true;}
if (minutes > 59) {minutes=0; hours++;   flag_hours = true;}
if (hours > 23)   {hours=0; GetTime();}
}
/**/

/*Servo включение */
void ServoOn ()
{
digitalWrite(SERVO_VCC, LOW);
xTimerStart(timerServoOff, 10);
}
/**/

/*Servo отключение */
void ServoOff()
{
digitalWrite(SERVO_VCC, HIGH);
if (flag_notice) IFTTTSend (String(ifttt_event), String(board_name[ROLE]) + " " + String(F("Position")), String(position_dec, 1), string_position + string_extreme);
flag_position = false;
}
/**/

/*Servo поворот */
void Position (int state)
{
// Все позиции в мс, шаг - четверть, 7,5 град.
int microsecond[24] = {2500, 1775, 1717, 1658, 1599, 1540, 1482, 1423, 1372, 1321, 1263, 1205, 1148, 1090, 1034, 977, 925, 873, 824, 775, 725, 675, 623, 571}; // все позиции в мс, шаг - четверть, 7,5 град.
if (flag_position) return;    
if (state > 21)  state = 21;       // Ограничение значения сверху
else if (state < 1)   state = 1;   // Ограничение значения снизу
      
memory.putInt("position", state);

flag_position = true;
servodrive.writeMicroseconds(microsecond[state]);      // Поворачиваем Servo на нужный угол
xTimerStart(timerServoOn, 10);                         // Подаем питание на короткое время
  
position_dif = state - position_old;                   // Вычисление изменения
position_old = state; 

position_dec = state / 4.0 + 0.75;                     // Преобразование положения в десятичное число
now_Store[ROLE].reg_position = (int) position_dec;
  
string_position = "⭕0.0";
     if (position_dif > 0) {string_position =  String("🔺") + "+" + String(position_dif/4.0, 1) ; time_msec_position = xTaskGetTickCount();} 
else if (position_dif < 0) {string_position =  String("🔻") + String(position_dif/4.0, 1);        time_msec_position = xTaskGetTickCount();}
}
/**/

/* Определение условий для выполнения регулировки мощности котла в зависимости от режима регулировки*/
int Situation ()
{
  if (control_mode ==0) return  0;
  if      (temp_dif > 0.25  && temp_trend > -0.125)    return -4;
  else if (temp_dif < -0.25 && temp_trend <  0.125)    return  4;
  else if (temp_dif > 0.125  && temp_trend >= 0.0)     return -2;
  else if (temp_dif < -0.125 && temp_trend <= 0.0)     return  2;
  else                                                 return  0;
}
/**/

/*Преобразования миллисекунд в строку с двоеточием чч:мм:сек */
String Hour_display (long time_msec)
{
int hour, minute, second;
String st_hour, st_minute, st_second;

hour = time_msec / 3600000;
minute = (time_msec % 3600000) / 60000;
second = ((time_msec % 3600000) % 60000) / 1000;

if (hour >= 10) st_hour = String (hour); else st_hour = "0" + String (hour);
if (minute >= 10) st_minute = String (minute); else st_minute = "0" + String (minute);
if (second >= 10) st_second = String (second); else st_second = "0" + String (second);

return st_hour + ":" + st_minute + ":" + st_second;
}
/**/

/*Кратковременное включение красного светодиода */
void Red_Off()
{
digitalWrite(LED_RED, LOW);
}
/**/

/*Кратковременное включение зеленого светодиода */
void Green_Off()
{
digitalWrite(LED_GREEN, LOW);
}
/**/

/*Кратковременное включение синего светодиода */
void Blue_Off()
{
digitalWrite(LED_BLUE, LOW);
}
/**/
