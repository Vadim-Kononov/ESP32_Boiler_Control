/*Функция проверки наличия связи, обработки принятых данных и отправки Now, */
void Cycle_A ()
{
//Проверка наличия связи с Room через сравнение отправленного и полученного хэша, установка флагов
if (now_Store[1].hash == now_Store[0].hash) {flag_Now_connected[1] = true ; now_Store[0].flag_Now_connected_1 = true;}
else {flag_Now_connected[1] = false; now_Store[0].flag_Now_connected_1 = false;}
//Проверка наличия связи с Outdoor  через сравнение отправленного и полученного хэша, установка флагов
if (now_Store[2].hash == now_Store[0].hash) {flag_Now_connected[2] = true ; now_Store[0].flag_Now_connected_2 = true;}
else {flag_Now_connected[2] = false; now_Store[0].flag_Now_connected_2 = false;}
//При наличии связи с Room
if (flag_Now_connected[1]) 
  {
  now_Store[ROLE].temp_room   = now_Store[1].temp_room; //Сохранение принятого значения в локальный массив
  now_Store[ROLE].hum_room    = now_Store[1].hum_room;  //Сохранение принятого значения в локальный массив
  now_Store[ROLE].press_room  = now_Store[1].press_room;//Сохранение принятого значения в локальный массив
  //Один раз после перезагрузки, приравнивается старая и текущая температура 
  if (flag_temp_room_old && now_Store[ROLE].temp_room != 0.0){flag_temp_room_old = false; temp_room_old = now_Store[ROLE].temp_room;}
  //Вычисление отклонения от заданной температуры
  temp_dif = now_Store[ROLE].temp_room - temp_zad;
  //Вычисление тренда за c начала цикла регулирования
  temp_trend = now_Store[ROLE].temp_room - temp_room_old; 
  //Вычисление тренда за один час
  //temp_trend = 60 * (60 * ((now_Store[ROLE].temp_room - temp_room_old) / ((xTaskGetTickCount() - time_msec_control) / 1000.0))); 
  
  //Заполнение строк числовыми значениями
  string_temp_room        = String(now_Store[ROLE].temp_room);
  string_temp_dif         = String(temp_dif); 
  string_temp_trend   = String(temp_trend); 
  } 
//При отсутсвии связи с Room
else 
  {
  //Заполнение строк символами отсутсвия связи
  string_temp_room        = String("❌");
  string_temp_dif         = String("❌"); 
  string_temp_trend   = String("❌"); 
  }
  //Заполнение строки символом ожидаемого направления поворота регулятора
  string_situation = "⭕0.0";
       if (Situation() > 0) string_situation =  String("🔺") + "+" + String(Situation()/4.0, 1); 
  else if (Situation() < 0) string_situation =  String("🔻") + String(Situation()/4.0, 1);

  //При наличии связи с Outdoor
  if (flag_Now_connected[2]) 
  {
  now_Store[ROLE].temp_outdoor  = now_Store[2].temp_outdoor;      //Сохранение принятого значения в локальный массив
  now_Store[ROLE].hum_outdoor   = now_Store[2].hum_outdoor;       //Сохранение принятого значения в локальный массив
  now_Store[ROLE].press_outdoor = now_Store[2].press_outdoor;     //Сохранение принятого значения в локальный массив
  }

  now_Store[0].hash  = random(100, 1000);                         //Изменение хэша
  xTimerChangePeriod(timerSendNow,  pdMS_TO_TICKS(TACT_time), 0); //Отправка Now данных
}
/**/

/*Функция обратного вызова при приеме данных ESPNOW*/
void Receiv_Now(const uint8_t * mac, const uint8_t *incomingData, int len)
{
//Мигание синим светодиодом
digitalWrite(LED_BLUE, HIGH); xTimerStart(timerBlue, 10);
//Заполнение структуры приема
memcpy(&now_Get, incomingData, sizeof(now_Get));
//Сохранение приятой структуры в элементе массива соответсвующему адресу платы, от которой поступили данные
now_Store[now_Get.unit] = now_Get;
}
/**/

/*Функция отправки данных через ESPNOW*/
void Send_Now ()
{
esp_err_t result = esp_now_send(NULL, (uint8_t *) &now_Store[ROLE], sizeof(now_Store[ROLE]));
}
/**/

/*Функция обратного вызова при отправке данных ESPNOW*/
void Sending_Complete_Now(const uint8_t *mac_addr, esp_now_send_status_t status)
{
//Мигание зеленым светодиодом
digitalWrite(LED_GREEN, HIGH); xTimerStart(timerGreen, 10);
}
/**/
