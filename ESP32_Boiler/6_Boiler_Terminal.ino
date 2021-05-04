/*Вывод в терминалы Serial, WiFiSerial, BTSerial*/
void Write(String string, uint8_t terminal)
{
/*B001 Serial | B010 WiFi | B100 BT*/
if bitRead(terminal, 0) DEBUG_PRINT(string);
if bitRead(terminal, 1) TelnetClient.print(string);
if bitRead(terminal, 2) SerialBT.print(string);
}
/**/

/*Вывод в терминалы Serial, WiFiSerial, BTSerial*/
void Writeln(String string, uint8_t terminal)
{
/*B001 Serial | B010 WiFi | B100 BT*/
if bitRead(terminal, 0) DEBUG_PRINTLN(string);
if bitRead(terminal, 1) TelnetClient.println(string);
if bitRead(terminal, 2) SerialBT.println(string);
}
/**/

/*Обработка строки принятой через Telnet или Bluetooth*/
void Terminal(String string, uint8_t terminal)
{
String string_1 = "", string_2 = "", string_3 = "", string_4 = "";

/*Парсинг входой строки на 3 элемента по "/"*/
if (string.indexOf("/") >=0) {string_1 = string.substring(0, string.indexOf("/")); string.remove(0, string.indexOf("/") + 1);} else {string.trim(); string_1 = string; string.remove(0, string.length());}
if (string.indexOf("/") >=0) {string_2 = string.substring(0, string.indexOf("/")); string.remove(0, string.indexOf("/") + 1);} else {string.trim(); string_2 = string; string.remove(0, string.length());}
if (string.indexOf("/") >=0) {string_3 = string.substring(0, string.indexOf("/")); string.remove(0, string.indexOf("/") + 1);} else {string.trim(); string_3 = string; string.remove(0, string.length());}
if (string.indexOf("/") >=0) {string_4 = string.substring(0, string.indexOf("/")); string.remove(0, string.indexOf("/") + 1);} else {string.trim(); string_4 = string; string.remove(0, string.length());}

/* Если был введен Password*/
if (flag_Terminal_pass)
{

/* "Password". Отключение пароля, если он включен. Режим триггера*/
if (string_1.equalsIgnoreCase("cbhLtpjr")) {flag_Terminal_pass = false; Writeln ("< ", terminal); return;}

/* "Login". Запись в память логина и пароля WiFi*/
else if (string_1.equalsIgnoreCase("Login"))
{
  if (!string_2.equalsIgnoreCase("") && !string_3.equalsIgnoreCase("")) {memory.putString("login", string_2); memory.putString("pass", string_3);}
  Writeln("> Login: " + memory.getString("login") + " | Pass: " + memory.getString("pass"), terminal);
}

/* "Reconnect". Переподключение WiFi по логину и паролю хранящимися в памяти*/
else if (string_1.equalsIgnoreCase("Reconnect"))
{
Writeln ("> Reconnect WiFi to: " + memory.getString("login"), terminal);
WiFi.disconnect(); WiFiConnect();
}

/* "Reset". Перезагрузка модуля*/
else if (string_1.equalsIgnoreCase("Reset")) {Writeln("> Reset", terminal); delay (250); ESP.restart();}

/* "Count". Вывод счетчиков перезагрузок и реконнектов*/
else if (string_1.equalsIgnoreCase("Count")) Writeln("> Count: Reset " + String(memory.getInt("countReset")) + " | " + "Wifi " + String(memory.getInt("countWifi")), terminal);

/* "CountRes". Сброс счетчиков перезагрузок и реконнектов*/
else if (string_1.equalsIgnoreCase("CountRes")) {memory.putInt("countReset", 0); memory.putInt("countWifi", 0); Writeln("> Count: Reset " + String(memory.getInt("countReset")) + " | " + "Wifi " + String(memory.getInt("countWifi")), terminal);}

/* "WiFi". Вывод текущих параметров WiFi*/
else if (string_1.equalsIgnoreCase("WiFi")) {Writeln("> " + String(WiFi.SSID()) + " : " + WiFi.channel() + " (" + String(WiFi.RSSI()) + ") " + WiFi.localIP().toString(), terminal);}

/* "Scan". Сканирование WiFi*/
else if (string_1.equalsIgnoreCase("Scan"))
{
Writeln("> Scan: ", terminal);
int n = WiFi.scanNetworks();
String str = "";
if (n == 0) {Writeln (">Сети не найдены", terminal);}
else {for (int i = 0; i < n; ++i) {str += String(i + 1); str += ". "; str += WiFi.SSID(i); str += " : "; str +=  WiFi.channel(i); str += " (";  str += WiFi.RSSI(i); str += ") \n";} Write(str, terminal);}
}

/* "Mem". Вывод количества свободной памяти*/
else if (string_1.equalsIgnoreCase("Mem")) {Writeln ("> Free memory: " + String(ESP.getFreeHeap()), terminal);}

/* "Now". Качество связи ESPNOW*/
else if (string_1.equalsIgnoreCase("Now"))
{
for (int i=0; i<BOARD_COUNT; i++) {if (i != ROLE) Writeln("> Unit: " + String(i) + "\tConnect: " + flag_Now_connected[i], terminal);}
}

/* "OTA". Разрешение загрузки по воздуху*/
else if (string_1.equalsIgnoreCase("OTA")) {ArduinoOTA.begin(); Writeln ("> OTA ON  " + String(ESP.getSketchSize()) + "/" + String(ESP.getFreeSketchSpace()) + "=" + String(float(ESP.getSketchSize())/ESP.getFreeSketchSpace()*100.0, 1) + " %", terminal);}

/* "Time". Вывод текущего времени, обращение к элементам структуры: timeinfo.tm_sec|timeinfo.tm_min|timeinfo.tm_hour и т.д., тип - int*/
else if (string_1.equalsIgnoreCase("Time")) {char time_str[50]; GetTime(); strftime(time_str, 50, "%A %d %b %Y %H:%M:%S", &timeinfo); Writeln("> " + String(time_str), terminal);}

/* "SetTemp". Заданная температура*/
else if (string_1.equalsIgnoreCase("SetTemp"))
{
if (!string_2.equalsIgnoreCase("")) {temp_zad = string_2.toFloat(); memory.putFloat("temp_zad", temp_zad);}
Writeln ("> Set temperature: " + String(temp_zad) + " ℃", terminal);
}

/* "Posit". Позиция регулятора*/
else if (string_1.equalsIgnoreCase("Posit"))
{
if (!string_2.equalsIgnoreCase("")) {position_int = string_2.toInt(); Position (position_int);}
Writeln ("> Position: " + String(position_int), terminal);
}

/* "Cycle". Время цикла регулятора*/
else if (string_1.equalsIgnoreCase("Cycle"))
{
if (!string_2.equalsIgnoreCase("")) {time_min_control = string_2.toInt(); memory.putInt("time_control", time_min_control);}
Writeln ("> Cycle: " + String(time_min_control) + " min.", terminal);
}

/* "Mode". Режим работы регулятора*/
else if (string_1.equalsIgnoreCase("Mode"))
{
if (!string_2.equalsIgnoreCase("")) {control_mode = string_2.toInt(); memory.putInt("control_mode",   control_mode);}
Writeln ("> Mode: " + String(control_mode), terminal);
}

/* "Notice". Вывод количества свободной памяти*/
else if (string_1.equalsIgnoreCase("Notice")) {flag_notice = !flag_notice; Writeln ("> Notice: " + String(flag_notice), terminal);}

/* "HourDown". Час снижения заданной температуры*/
else if (string_1.equalsIgnoreCase("HourDown"))
{
if (!string_2.equalsIgnoreCase("")) {hour_down = string_2.toInt(); memory.putInt("hour_down", hour_down);}
Writeln ("> Hour Down: " + String(hour_down) + " h", terminal);
}

/* "HourUp". Час повышения заданной температуры*/
else if (string_1.equalsIgnoreCase("HourUp"))
{
if (!string_2.equalsIgnoreCase("")) {hour_up = string_2.toInt(); memory.putInt("hour_up", hour_up);}
Writeln ("> Hour Up: " + String(hour_up) + " h", terminal);
}

/* "Help".*/
else if (string_1.equalsIgnoreCase("?") || string_1.equalsIgnoreCase("Help"))
{Writeln (">" +
String("\n| Scan | WiFi | Reconnect | Reset |") +
String("\n| Login/<SSID>/<Password> |") +
String("\n| Time | Mem  | OTA |") +
String("\n| Count | CountRes |") +
String("\n| Notice | HourDown/<0÷23> | HourUp/<0÷23>") +
String("\n| Cycle/<15|30|60> | Mode/<0÷3>") +
String("\n| Posit/<1÷21> x=P/4+0.75") +
String("\n| SetTemp/<21.0÷25.0> |") +
String("\n| ? | Help |") +
String("\n>")
, terminal);}

/* Все остальное*/
else
{
Writeln ("1>/" + string_1, terminal);
Writeln ("2>/" + string_2, terminal);
Writeln ("3>/" + string_3, terminal);
Writeln ("4>/" + string_4, terminal);
Writeln (">>/" + string, terminal); 
}
}

/* Обработка пароля после перезагрузки*/
if (!flag_Terminal_pass) {if (!string_1.compareTo("cbhLtpjr")) {flag_Terminal_pass = true; Writeln ("> ", terminal);} else Writeln ("Password > ", terminal);}
}
