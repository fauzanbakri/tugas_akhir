#include "painlessMesh.h"
#include <Wire.h>
#include <RtcDS3231.h>
#include <SD.h>
#include <SPI.h>
#define   MESH_PREFIX     "whateverYouLike"
#define   MESH_PASSWORD   "somethingSneaky"
#define   MESH_PORT       5555
#define countof(a) (sizeof(a) / sizeof(a[0]))

RtcDS3231<TwoWire> Rtc(Wire);

Scheduler userScheduler;
painlessMesh  mesh;
int timetemp, date_temp, month_temp, year_temp;
File myFile, myFile1, myFile2, myFile3, myFile4, myFile5, myFile6;

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
  RtcDateTime now = Rtc.GetDateTime();
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
//    Serial.print("check = ");
//    Serial.print(check);
//    Serial.print("\n");
//    Serial.print("id = ");
//    Serial.print(id);
//    Serial.print("\n");  
  if  (String(check)==id) {
    int timelast = timetemp+120;
    int timenow = (hour(now).toInt() * 3600) + (minute(now).toInt() * 60) + second(now).toInt();
    int date_now = date(now).toInt();
    int month_now = month(now).toInt();
    int year_now = year(now).toInt();
    Serial.print("timelast = ");
    Serial.print(timelast);
    Serial.print("\n");
    Serial.print("timenow = ");
    Serial.print(timenow);
    Serial.print("\n");
    if  (timenow > timelast || date_now > date_temp || month_now > month_temp || year_now > year_temp){
      SD.remove("from.csv");
      timetemp = (hour(now).toInt() * 3600) + (minute(now).toInt() * 60) + second(now).toInt();
    }else{
      Serial.print("wait 30 second to accept from same node\n");
    }
  }else{
//    SD.remove("from.csv");
  myFile2 = SD.open("from.csv", FILE_WRITE);
  if (myFile2) {
    myFile2.print(from);
  } else {
    Serial.println("error write to from.csv");
  }
  myFile2.close();
//  Serial.print("check2 = ");
//  Serial.print(check);
//  Serial.print("\n");
//  Serial.print("id2 = ");
//  Serial.print(id);
//  Serial.print("\n");
  timetemp = (hour(now).toInt() * 3600) + (minute(now).toInt() * 60) + second(now).toInt();
  date_temp = date(now).toInt();
  month_temp = month(now).toInt();
  year_temp = year(now).toInt();
  Serial.print("Timetemp = ");
  Serial.print(timetemp);
  Serial.print("\n");
  while (count < c) {
    row = getValue(msg, '#', count);
    row_item = getValue(row, ',', 2);
    id_check = String(row_item);
    if (id_check == id_self) {
      Serial.print("can't write this row to csv because this data from here = ");
      Serial.println(row_item);
    }
    else {
      myFile4 = SD.open("rcvdata.csv", FILE_WRITE);
      if (myFile4) {
        myFile4.print(row);
        myFile4.println("#");
        
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

void setup() {
  Serial.begin(9600);
  Serial.print("compiled: ");
  Serial.print(__DATE__);
  Serial.println(__TIME__);
  Wire.begin(5, 4);
  Rtc.Begin();
  RtcDateTime compiled = RtcDateTime(__DATE__, __TIME__);
  printDate(compiled);
  printTime(compiled);
  hour(compiled);
  minute(compiled);
  second(compiled);
  year(compiled);
  month(compiled);
  date(compiled);
  Serial.println();
  if (!Rtc.IsDateTimeValid())
  {
    if (Rtc.LastError() != 0)
    {
      Serial.print("RTC communications error = ");
      Serial.println(Rtc.LastError());
    }
    else
    {
      Serial.println("RTC lost confidence in the DateTime!");
      Rtc.SetDateTime(compiled);
    }
  }
  if (!Rtc.GetIsRunning())
  {
    Serial.println("RTC was not actively running, starting now");
    Rtc.SetIsRunning(true);
  }
  RtcDateTime now = Rtc.GetDateTime();
  if (now < compiled)
  {
    Serial.println("RTC is older than compile time!  (Updating DateTime)");
    Rtc.SetDateTime(compiled);
  }
  else if (now > compiled)
  {
    Serial.println("RTC is newer than compile time. (this is expected)");
  }
  else if (now == compiled)
  {
    Serial.println("RTC is the same as compile time! (not expected but all is fine)");
  }
  Rtc.Enable32kHzPin(false);
  Rtc.SetSquareWavePin(DS3231SquareWavePin_ModeNone);

  Serial.print("Initializing SD card...");
  if (!SD.begin(15)) {
    Serial.println("initialization failed!");
    return;
  }
  Serial.println("initialization done.");

  //mesh.setDebugMsgTypes( ERROR | MESH_STATUS | CONNECTION | SYNC | COMMUNICATION | GENERAL | MSG_TYPES | REMOTE ); // all types on
  //  mesh.setDebugMsgTypes( ERROR | STARTUP );  // set before init() so that you can see startup messages

  mesh.init( MESH_PREFIX, MESH_PASSWORD, &userScheduler, MESH_PORT );
  mesh.onReceive(&receivedCallback);
  mesh.onNewConnection(&newConnectionCallback);
  mesh.onChangedConnections(&changedConnectionCallback);
  mesh.onNodeTimeAdjusted(&nodeTimeAdjustedCallback);

  userScheduler.addTask( taskSendMessage );
  taskSendMessage.enable();
}

void loop() {

  if (!Rtc.IsDateTimeValid())
  {
    if (Rtc.LastError() != 0)
    {
      Serial.print("RTC communications error = ");
      Serial.println(Rtc.LastError());
    }
    else
    {
      Serial.println("RTC lost confidence in the DateTime!");
    }
  }
  RtcDateTime now = Rtc.GetDateTime();
  int interval = minute(now).toInt();
  int interval2 = second(now).toInt();
  if (interval2 == 0) {
    SD.remove("datasend.csv");
    writeToCsv();
  }
  delay(1000);
  mesh.update();
}

void writeToCsv() {
  myFile5 = SD.open("tempdata.csv", FILE_WRITE);
  if (myFile5) {
    RtcDateTime now = Rtc.GetDateTime();
    String timestamp = printTime(now);
    String datestamp = printDate(now);
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
    RtcDateTime now = Rtc.GetDateTime();
    String timestamp = printTime(now);
    String datestamp = printDate(now);
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

String printDate(const RtcDateTime& dt)
{
  char datestring[20];
  sprintf(datestring, "%02d/%02d/%d",
          dt.Month(),
          dt.Day(),
          dt.Year() );
  Serial.println(datestring);
  return datestring;
}
String printTime(const RtcDateTime& dt)
{
  char timestring[20];
  sprintf(timestring, "%02d:%02d",
          dt.Hour(),
          dt.Minute() );
  Serial.println(timestring);
  return timestring;
}
String hour(const RtcDateTime& dt)
{
  char timestring[20];
  sprintf(timestring, "%d",
          dt.Hour() );
  //    Serial.println(timestring);
  return timestring;
}
String minute(const RtcDateTime& dt)
{
  char timestring[20];
  sprintf(timestring, "%d",
          dt.Minute() );
  //    Serial.println(timestring);
  return timestring;
}

String second(const RtcDateTime& dt)
{
  char timestring[20];
  sprintf(timestring, "%d",
          dt.Second() );
  //    Serial.println(timestring);
  return timestring;
}
String year(const RtcDateTime& dt)
{
  char timestring[20];
  sprintf(timestring, "%d",
          dt.Year() );
  //    Serial.println(timestring);
  return timestring;
}
String month(const RtcDateTime& dt)
{
  char timestring[20];
  sprintf(timestring, "%d",
          dt.Month() );
  //    Serial.println(timestring);
  return timestring;
}
String date(const RtcDateTime& dt)
{
  char timestring[20];
  sprintf(timestring, "%d",
          dt.Day() );
  //    Serial.println(timestring);
  return timestring;
}



