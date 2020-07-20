#include "painlessMesh.h"
#include <ESP8266WiFi.h>
#include <PubSubClient.h>
#include <WiFiClient.h>
#include <string.h>
#include <Wire.h>
#include <SD.h>
#include <SPI.h>
#include <MQ7.h>
#include <DHT.h>
#include <MQ135.h>
#include <TinyGPS++.h>
#include <SoftwareSerial.h>

#define MESH_PREFIX "AirQualityMonitoring"
#define MESH_PASSWORD "12345678"
#define MESH_PORT 5555
#define countof(a) (sizeof(a) / sizeof(a[0]))

int dhtpin = 0;
TinyGPSPlus gps;
DHT dht(dhtpin, DHT22);
SoftwareSerial ss(5, 2);
Scheduler userScheduler;
int pin_pm25 = 4;
int mux = 3;

byte buff[2];
//unsigned long duration1;
//unsigned long starttime1;
//unsigned long endtime1;
unsigned long duration2;
unsigned long starttime2;
unsigned long endtime2;
unsigned long sampletime_ms = 1000;
//unsigned long lowpulseoccupancy1 = 0;
unsigned long lowpulseoccupancy2 = 0;
float temperature, hum;
//float ratio1 = 0;
//float concentration1 = 0;
float ratio2 = 0;
float concentration2 = 0;
painlessMesh  mesh;
const char* ssid = "fznn";
const char* password = "fauzan97";
const char* mqtt_server = "192.168.43.170";
const char* mqtt_username = "fauzan";
const char* mqtt_password = "fauzan97";

WiFiClient wifiClient;
PubSubClient client(mqtt_server, 1883, wifiClient); 

MQ135 gasSensor = MQ135(A0);
MQ7 mq7(A0,5.0);
int interval_start;
int interval_start_wr;
float rzero = gasSensor.getRZero();
int ppm, ppm_co;
int timetemp, date_temp, month_temp, year_temp;
File myFile, myFile1, myFile2, myFile3, myFile4, myFile5, myFile6, myFile7;
float latitude , longitude;
int year , month , date, hour , minute , second;
String date_str , time_str , lat_str , lng_str;
int date_inc;

void sendMessage() ;

Task taskSendMessage( TASK_SECOND * 1 , TASK_FOREVER, &sendMessage );

String getValue(String data, char separator, int index)
{
  int found = 0;
  int strIndex[] = {0, -1};
  int maxIndex = data.length() - 1;

  for (int i = 0; i <= maxIndex && found <= index; i++) {
    if (data.charAt(i) == separator || i == maxIndex) {
      found++;
      strIndex[0] = strIndex[1] + 1;
      strIndex[1] = (i == maxIndex) ? i + 1 : i;
    }
  }

  return found > index ? data.substring(strIndex[0], strIndex[1]) : "";
}

void sendMessage() {
  String msg, msg1, msg2;
  myFile = SD.open("datasend.csv", FILE_READ);
  while (myFile.available()) {
    msg1 += (char)myFile.read();
  }
  myFile.close();
  myFile1 = SD.open("rcvdata.csv", FILE_READ);
  while (myFile1.available()) {
    msg2 += (char)myFile1.read();
  }
  myFile1.close();
  msg = msg1+msg2;
  mesh.sendBroadcast( msg );
  taskSendMessage.setInterval( random( TASK_SECOND * 1, TASK_SECOND * 1 ));
}

void receivedCallback( uint32_t from, String &msg ) {
  Serial.printf("Received from %u msg=%s\n", from, msg.c_str());
  int count = 0;
  char char_append;
  String str = msg;
  String id_self = String(mesh.getNodeId());
  String id = String(from);
  String row, row_item, check, id_check, data_from;
  int n = 0, c = 0;
  a:
  if (str[n] == '#')
    c++;
  n++;
  if (str[n])
    goto a;
  myFile3 = SD.open("from.csv", FILE_READ);
  if (myFile3) {
    while (myFile3.available()) {
      data_from = (char)myFile3.read();
      check += data_from;
    }
  } else {
    Serial.println("error opening from.csv");
  }
  myFile3.close();
  if  (String(check)==id) {
    int timelast = timetemp+60000;
    int timenow = millis();
//    int date_now = date;
//    int month_now = month;
//    int year_now = year;
    Serial.print("timelast = ");
    Serial.print(timelast);
    Serial.print("\n");
    Serial.print("timenow = ");
    Serial.print(timenow);
    Serial.print("\n");
    if  (timenow > timelast){
      SD.remove("from.csv");
      timetemp = millis();
    }else{
      Serial.print("wait 1 minute to accept from same node\n");
    }
  }else{
  myFile2 = SD.open("from.csv", FILE_WRITE);
  if (myFile2) {
    myFile2.print(from);
  } else {
    Serial.println("error write to from.csv");
  }
  myFile2.close();
  timetemp = millis();
//  date_temp = date;
//  month_temp = month;
//  year_temp = year;
  Serial.print("Timetemp = ");
  Serial.print(timetemp);
  Serial.print("\n");
  while (count < c) {
    row = getValue(msg, '#', count);
    row_item = getValue(row, ',', 9);
    id_check = String(row_item);
    if (id_check == id_self) {
      Serial.print("can't write this row to csv because this data from here = ");
      Serial.println(row_item);
    }
    else {
      myFile4 = SD.open("rcvdata.csv", FILE_WRITE);
      if (myFile4) {
        myFile4.println(row + "#");
//        myFile4.println("#");
        
      }else{
      Serial.println("Failed write rcvdata.csv");  
      }
      myFile4.close();
    }
    count++;
  }
  }

}

void newConnectionCallback(uint32_t nodeId) {
  Serial.printf("--> startHere: New Connection, nodeId = %u\n", nodeId);
}

void changedConnectionCallback() {
  Serial.printf("Changed connections\n");
}

void nodeTimeAdjustedCallback(int32_t offset) {
  Serial.printf("Adjusted time %u. Offset = %d\n", mesh.getNodeTime(), offset);
}
void setup_wifi() {
  Serial.println();
  Serial.print("Connecting to ");
  Serial.println(ssid);
  WiFi.begin(ssid, password);
  delay(5000);
}
int a;
void setup() {
  Serial.begin(115200);
  pinMode(3, FUNCTION_3);
  pinMode(mux, OUTPUT);
  pinMode(dhtpin, INPUT);
  dht.begin();
  ss.begin(9600);
//  pinMode(pin_pm10,INPUT);
//  starttime1 = millis();
  pinMode(pin_pm25,INPUT);
  starttime2 = millis();
  interval_start = millis();
  interval_start_wr = millis();
  
  Serial.print("Initializing SD card...");
  if (!SD.begin(15)) {
    Serial.println("initialization failed!");
    return;
  }
  Serial.println("initialization done.");
//  pinMode(16, INPUT);
  String bootmode, boot;
  myFile3 = SD.open("bootmode.txt", FILE_READ);
  if (myFile3) {
    while (myFile3.available()) {
      boot = (char)myFile3.read();
      bootmode += boot;
    }
  } else {
    Serial.println("error opening bootmode.txt");
  }
  int check_boot = bootmode.toInt();
  
  if (check_boot == 1){
   setup_wifi(); 
   a = 1;
  }else{
   a = 0;
  //  mesh.setDebugMsgTypes( ERROR | MESH_STATUS | CONNECTION | SYNC | COMMUNICATION | GENERAL | MSG_TYPES | REMOTE ); // all types on
  //  mesh.setDebugMsgTypes( ERROR | STARTUP );  // set before init() so that you can see startup messages

  mesh.init( MESH_PREFIX, MESH_PASSWORD, &userScheduler, MESH_PORT);
  mesh.onReceive(&receivedCallback);
  mesh.onNewConnection(&newConnectionCallback);
  mesh.onChangedConnections(&changedConnectionCallback);
  mesh.onNodeTimeAdjusted(&nodeTimeAdjustedCallback);

  userScheduler.addTask( taskSendMessage );
  taskSendMessage.enable();
  }
}

void loop() {
  if (a != 1){
    mesh.update();
    //scanwifi();
  } else {
    mqtt();
  }
//  if (restart_esp == 1){
//    SD.remove("bootmode.txt");
//    myFile7 = SD.open("bootmode.txt", FILE_WRITE);
//    if (myFile7) {
//      myFile7.println("1");
//      Serial.println("done write bootmode.txt");
//      myFile7.close();
//    } else {
//      Serial.println("error writing bootmode.txt");
//    }
//  ESP.restart();
//  }

//  
//  if (digitalRead(16) != a){
//    ESP.restart();
//  }
  gps_read2();
  dht22();
  mq135_mq7();
  int interval_end = millis();
  int interval_end_wr = millis();
  if (interval_end - interval_start > 1000) {
  Serial.print("Date: ");
  Serial.print(date_str);
  Serial.print(", Time: ");
  Serial.print(time_str);
  Serial.print(", Temperature: ");
  Serial.print(temperature);
  Serial.print(", Humidity: ");
  Serial.print(hum);
  Serial.print(", CO: ");
  Serial.print(ppm_co);
  Serial.print(", CO2: ");
  Serial.print(ppm);
  Serial.print(", PM2.5: ");
  Serial.print(concentration2);
  Serial.print(", ID: ");
  Serial.print(mesh.getNodeId());
  Serial.print(", Latitude: ");
  Serial.print(lat_str);
  Serial.print(", Longitude: ");
  Serial.println(lng_str);
  dsm501();
  if (interval_end_wr - interval_start_wr > 60000){
    SD.remove("datasend.csv");
    Serial.print("Write");
    writeToCsv();
    interval_start_wr = millis();
  }
    interval_start = millis();
  }
}

void writeToCsv() {
  String timestamp = time_str;
  String datestamp = date_str;
  myFile5 = SD.open("tempdata.csv", FILE_WRITE);
  if (myFile5) {
    myFile5.print(ppm);
    myFile5.print(",");
    myFile5.print(ppm_co);
    myFile5.print(",");
    myFile5.print(temperature);
    myFile5.print(",");
    myFile5.print(hum);
    myFile5.print(",");
    myFile5.print(concentration2);
    myFile5.print(",");
    myFile5.print(lat_str);
    myFile5.print(",");
    myFile5.print(lng_str);
    myFile5.print(",");
    myFile5.print(datestamp);
    myFile5.print(",");
    myFile5.print(timestamp);
    myFile5.print(",");
    myFile5.print(mesh.getNodeId());
    myFile5.print(",");
    myFile5.println("end#");
    Serial.println("done write tempdata.csv");
  } else {
    Serial.println("error writing tempdata.csv");
  }
  myFile5.close();

  myFile6 = SD.open("datasend.csv", FILE_WRITE);
  if (myFile6) {
    myFile6.print(ppm);
    myFile6.print(",");
    myFile6.print(ppm_co);
    myFile6.print(",");
    myFile6.print(temperature);
    myFile6.print(",");
    myFile6.print(hum);
    myFile6.print(",");
    myFile6.print(concentration2);
    myFile6.print(",");
    myFile6.print(lat_str);
    myFile6.print(",");
    myFile6.print(lng_str);
    myFile6.print(",");
    myFile6.print(datestamp);
    myFile6.print(",");
    myFile6.print(timestamp);
    myFile6.print(",");
    myFile6.print(mesh.getNodeId());
    myFile6.print(",");
    myFile6.println("end#");
    Serial.println("done write datasend.csv");
  } else {
    Serial.println("error writing datasend.csv");
  }
  myFile6.close();
}

void mq135_mq7() { 
    digitalWrite(mux, LOW);
    ppm = gasSensor.getPPM();
    digitalWrite(mux, HIGH);
    ppm_co = mq7.getPPM();
}
void gps_read2()
{

  while (ss.available() > 0)
    if (gps.encode(ss.read()))
    {
      if (gps.location.isValid())
      {
        latitude = gps.location.lat();
        lat_str = String(latitude , 6);
        longitude = gps.location.lng();
        lng_str = String(longitude , 6);
      }

      if (gps.date.isValid())
      {
        date_str = "";
        date = gps.date.day() + date_inc;
        month = gps.date.month();
        year = gps.date.year();

        if (date < 10)
          date_str = '0';
        date_str += String(date);

        date_str += "/";

        if (month < 10)
          date_str += '0';
        date_str += String(month);

        date_str += "/";

        if (year < 10)
          date_str += '0';
        date_str += String(year);
      }

      if (gps.time.isValid())
      {
        time_str = "";
        hour = gps.time.hour();
        minute = gps.time.minute();
        second = gps.time.second();
        
        if (minute > 59)
        {
          minute = minute - 60;
          hour = hour + 1;
        }
        hour = (hour + 8);
        if (hour > 23){
          hour = hour - 24;
          date_inc = 1;
        }else{
          date_inc = 0;   
        }

        if (hour < 10)
          time_str = '0';
        time_str += String(hour);

        time_str += ":";

        if (minute < 10)
          time_str += '0';
        time_str += String(minute);

        time_str += ":";

        if (second < 10)
          time_str += '0';
        time_str += String(second);
      }

    }
  delay(100);
}
void dsm501() {
//  duration1 = pulseIn(pin_pm10, LOW);
  duration2 = pulseIn(pin_pm25, LOW);
//  lowpulseoccupancy1 += duration1;
  lowpulseoccupancy2 += duration2;
//  endtime1 = millis();
  endtime2 = millis();
//  if ((endtime1-starttime1) > sampletime_ms)
//  {
//    ratio1 = (lowpulseoccupancy1-endtime1+starttime1 + sampletime_ms)/(sampletime_ms*10.0);  // Integer percentage 0=>100
//    concentration1 = 1.1*pow(ratio1,3)-3.8*pow(ratio1,2)+520*ratio1+0.62; // using spec sheet curve
//      if (ratio1 > 400000){
//      concentration1 = 0;
//      ratio1 = 0;
//    }
//    lowpulseoccupancy1 = 0;
//    starttime1 = millis();
//  }
    if ((endtime2-starttime2) > sampletime_ms)
  {
    ratio2 = (lowpulseoccupancy2-endtime2+starttime2 + sampletime_ms)/(sampletime_ms*10.0);  // Integer percentage 0=>100
    concentration2 = 1.1*pow(ratio2,3)-3.8*pow(ratio2,2)+520*ratio2+0.62; // using spec sheet curve
    if (ratio2 > 400000){
      concentration2 = 0;
      ratio2 = 0;
    }
    lowpulseoccupancy2 = 0;
    starttime2 = millis();
  }
}
void dht22() {
    hum = dht.readHumidity();
    temperature= dht.readTemperature();
}
void mqtt() {
  String mqtt_msg, rows, carbondioxide,carbonmonoxide,temper,humidi,pm25dsm,latt,longit,daterecord,timerecord,id_esp;
  int m=0, ct=0, count=0;
    myFile1 = SD.open("rcvdata.csv", FILE_READ);
  while (myFile1.available()) {
    mqtt_msg += (char)myFile1.read();
  }
  myFile1.close();

  b:
  if (mqtt_msg[m] == '#')
    ct++;
  m++;
  if (mqtt_msg[m])
    goto b;
  Serial.print("COUNT =");
  Serial.println(count);
    Serial.print("CT =");
  Serial.println(ct);
  if(client.connect("send", mqtt_username, mqtt_password)){
    Serial.println("mqtt connect");;
  }
  while (count - 1 < ct) {
    rows = getValue(mqtt_msg, '#', count);
    carbondioxide = getValue(rows, ',', 0);
    carbonmonoxide = getValue(rows, ',', 1);
    temper = getValue(rows, ',', 2);
    humidi = getValue(rows, ',', 3);
    pm25dsm = getValue(rows, ',', 4);
    latt = getValue(rows, ',', 5);
    longit = getValue(rows, ',', 6);
    daterecord = getValue(rows, ',', 7);
    timerecord = getValue(rows, ',', 8);
    id_esp = getValue(rows, ',', 9);
    Serial.println(String(carbondioxide)); 
    Serial.println(String(carbonmonoxide));
    Serial.println(String(temper)); 
    Serial.println(String(humidi));
    Serial.println(String(pm25dsm));
    Serial.println(String(latt));
    Serial.println(String(longit));
    Serial.println(String(daterecord));
    Serial.println(String(timerecord));    
    Serial.println(String(id_esp));
  
    client.publish("gateway/co2", carbondioxide.c_str());
    client.publish("gateway/co", carbonmonoxide.c_str());
    client.publish("gateway/temperature", temper.c_str());
    client.publish("gateway/humidity", humidi.c_str());
    client.publish("gateway/pm2.5", pm25dsm.c_str());
    client.publish("gateway/lattitude", latt.c_str());
    client.publish("gateway/longitude", longit.c_str());
    client.publish("gateway/date", daterecord.c_str());  
    client.publish("gateway/time", timerecord.c_str());
    client.publish("gateway/id", id_esp.c_str());
    client.publish("gateway/date_send", date_str.c_str());
    client.publish("gateway/time_send", time_str.c_str());       

    count++;
  }
    SD.remove("bootmode.txt");
    ESP.restart();
}
