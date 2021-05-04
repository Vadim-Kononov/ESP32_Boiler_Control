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

digitalWrite(LED_RED, HIGH); xTimerStart(timerRed_Off, 0);

BME280::TempUnit tempUnit(BME280::TempUnit_Celsius);
BME280::PresUnit presUnit(BME280::PresUnit_Pa);

bme.read(now_Store[ROLE].press_outdoor, now_Store[ROLE].temp_outdoor, now_Store[ROLE].hum_outdoor, tempUnit, presUnit);   

now_Store[ROLE].temp_outdoor = now_Store[ROLE].temp_outdoor + temp_correction;                                    //Коррекция температуры
now_Store[ROLE].press_outdoor = now_Store[ROLE].press_outdoor * 0.00750063755419211 + press_correction;           //Коррекция давления
now_Store[ROLE].hum_outdoor = now_Store[ROLE].hum_outdoor + hum_correction;                                       //Коррекция влажности

}
/**/



/*Cycle_B, длительные операции*/
if (flag_Cycle_B)
{
flag_Cycle_B = false;
WiFiReconnect ();
}
/**/



}
