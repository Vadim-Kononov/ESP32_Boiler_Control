/*Функция подключения*/
void mqtt_Connect()
{
if (flag_MQTT_connected)        {DEBUG_PRINTLN(F("MQTT is connected")); return;}
if (!flag_WiFi_connected)       {DEBUG_PRINTLN(F("WiFi not connected")); return;}
if (!Ping.ping("8.8.8.8", 1))   {DEBUG_PRINTLN(F("MQTT: Internet not connected")); return;}
DEBUG_PRINTLN("MQTT connecting ...");
mqttClient.connect();
}
/**/

/*Обработчик события подключения*/
/*Здесь подиска на топики*/
void mqtt_Connected_Complete(bool sessionPresent)
{
flag_MQTT_connected = true;
DEBUG_PRINTLN(F("Connected to MQTT :)"));
mqttClient.subscribe("position_int", 0);                                        // Подписка на положение Sit630
mqttClient.subscribe("mode", 0);                                                // Подписка на режим работы регулятора 0 - ручной, 1, 2 - автоматический
mqttClient.subscribe("time_control", 0);                                        // Подписка на время цикла работы регулятора
mqttClient.subscribe("temp_zad", 0);                                            // Подписка на заданную температуру
}
/**/

/*Обработчик события обрыва связи*/
void mqtt_Disconnect_Complete(AsyncMqttClientDisconnectReason reason)
{
/*Флаг обрыва связи*/
flag_MQTT_connected = false;
DEBUG_PRINTLN(F("Disconnected from MQTT :("));
}
/**/

/*Обработчик события подписки*/
void mqtt_Subscribe_Complete(uint16_t packetId, uint8_t qos)
{
DEBUG_PRINTLN(String(F("Subscription confirmed, packetId:\t")) + String(packetId) + String("\tqos:\t") + String(qos));
}
/**/

/*Обработчик события отписки*/
void mqtt_Unsubscribe_Complete(uint16_t packetId)
{
DEBUG_PRINTLN(String(F("Unsubscribe confirmed, packetId:\t")) + String(packetId));
}
/**/

/*Обработчик события публикации*/
void mqtt_Publishe_Complete(uint16_t packetId)
{
DEBUG_PRINTLN(String(F("Publication confirmed, packetId:\t")) + String(packetId));
}
/**/

/*Обработчик события приема сообщения*/
/*Здесь обработка принятых сообщений*/
void mqtt_Receiving_Complete(char* topic, char* payload, AsyncMqttClientMessageProperties properties, size_t len, size_t index, size_t total)
{
String Payload = String (payload).substring(0, len); 
if (String(topic).equalsIgnoreCase("position_int"))  {if (position_int !=  Payload.toInt() && !flag_position) {position_int =  Payload.toInt(); Position (position_int);}}
if (String(topic).equalsIgnoreCase("mode"))          {control_mode =  Payload.toInt();   memory.putInt("control_mode",   control_mode);}     // Если опубликован режим работы регулятора 
if (String(topic).equalsIgnoreCase("time_control"))  {time_min_control =  Payload.toInt(); memory.putInt("time_control", time_min_control);} // Если опубликовано время цикла регулятора
if (String(topic).equalsIgnoreCase("temp_zad"))                                                                                              // Если опубликована заданная температура в комнате
{
 char x[10];
 Payload.toCharArray(x, Payload.length() + 1);
 temp_zad = atof(x);
 memory.putFloat("temp_zad", temp_zad);
}
}
/**/

/*Функция отправки сообщений MQTT*/
void mqtt_Send ()
{
if (flag_MQTT_connected)
{
mqttClient.publish("temp_room",       0, false, string_temp_room.c_str());                                                              // Публикация температуры в комнате
mqttClient.publish("temp_dif",        0, false, string_temp_dif.c_str());                                                               // Публикация разности тепературы в комнате
mqttClient.publish("temp_water",      0, false, String(now_Store[ROLE].temp_water, 1).c_str());                                         // Публикация температуры воды в котле
mqttClient.publish("temp_gas",        0, false, String(now_Store[ROLE].temp_gas, 1).c_str());                                           // Публикация температуру отходящих газов

mqttClient.publish("hum_room",        0, false, String(now_Store[ROLE].hum_room, 1).c_str());                                           // Публикация влажности в комнате
mqttClient.publish("press_room",      0, false, String(now_Store[ROLE].press_room, 1).c_str());                                         // Публикация даления в комнате
mqttClient.publish("temp_outdoor",    0, false, String(now_Store[ROLE].temp_outdoor, 1).c_str());                                       // Публикация температуры во дворе
mqttClient.publish("hum_outdoor",     0, false, String(now_Store[ROLE].hum_outdoor, 1).c_str());                                        // Публикация влажности во дворе
mqttClient.publish("press_outdoor",   0, false, String(now_Store[ROLE].press_outdoor, 1).c_str());                                      // Публикация давления во дворе

mqttClient.publish("temp_trend",      0, false, string_temp_trend.c_str());                                                             // Публикация тренда

mqttClient.publish("elapsed_control", 0, false, String(xTaskGetTickCount() - time_msec_control).c_str());                               // Публикация времени от последней регулировки для диаграммы
mqttClient.publish("remained_control",0, false, String(60000 * time_min_control - (xTaskGetTickCount() - time_msec_control)).c_str());  // Публикация оставшегося времени до следующей регулировки для диаграммы
mqttClient.publish("string_status_1", 0, false, string_status_1.c_str());                                                                // Публикация строки из положения и счетчика поворотов
mqttClient.publish("string_status_2", 0, false, string_status_2.c_str());                                                                // Публикация последнего шага и времени от последнего вращения

if (!flag_position) mqttClient.publish("position",        0, false, String(position_int).c_str());                                      // Публикация текущего положения Sit630
else                mqttClient.publish("position",        0, false, String("#").c_str());                                               // Если выполняется поворот то публикация #

mqttClient.publish("position_dec",    0, false, String(position_dec, 2).c_str());                                                       // Положения десятичным числом
mqttClient.publish("position_dif",    0, false, String((position_dif/4.0), 2).c_str());                                                 // Положения десятичным числом
mqttClient.publish("turn_counter",    0, false, String(turn_counter).c_str());                                                          // Счетчика поворотов

mqttClient.publish("mode",            0, false, String(control_mode).c_str());                                                          // Публикация режима
mqttClient.publish("time_control",    0, false, String(time_min_control).c_str());                                                      // Публикация времени прошедшего от управления
mqttClient.publish("temp_zad",        0, false, String(temp_zad).c_str());                                                              // Публикация заданной температуры

mqttClient.publish("temp_saved7",     0, false, String(temp_room_saved [7]).c_str());      
mqttClient.publish("temp_saved6",     0, false, String(temp_room_saved [6]).c_str());      
mqttClient.publish("temp_saved5",     0, false, String(temp_room_saved [5]).c_str());                                                 // Публикация сохраненной температуры для графика
mqttClient.publish("temp_saved4",     0, false, String(temp_room_saved [4]).c_str());                                                 // Публикация сохраненной температуры для графика
mqttClient.publish("temp_saved3",     0, false, String(temp_room_saved [3]).c_str());                                                 // Публикация сохраненной температуры для графика
mqttClient.publish("temp_saved2",     0, false, String(temp_room_saved [2]).c_str());                                                 // Публикация сохраненной температуры для графика
mqttClient.publish("temp_saved1",     0, false, String(temp_room_saved [1]).c_str());                                                 // Публикация сохраненной температуры для графика
mqttClient.publish("temp_saved0",     0, false, String(temp_room_saved [0]).c_str());                                                 // Публикация сохраненной температуры для графика

mqttClient.publish("min_in_day",      0, false, String(string_hours + ":" + string_minutes + ":" + string_seconds).c_str());          // Публикация текущего времени
}
}
/**/
