#include<avr/wdt.h> /* Header for watchdog timers in AVR */

#define ONE_WIRE_PIN 7

byte data[9];

unsigned long lastRequest = 0;
bool conversionRunning = false;

// ================= 1-Wire =================

bool resetPulse() {

  pinMode(ONE_WIRE_PIN, OUTPUT);
  digitalWrite(ONE_WIRE_PIN, LOW);
  delayMicroseconds(480);

  pinMode(ONE_WIRE_PIN, INPUT_PULLUP);
  delayMicroseconds(70);

  bool presence = !digitalRead(ONE_WIRE_PIN);

  delayMicroseconds(410);

  return presence;
}

void writeBit(bool bit) {

  pinMode(ONE_WIRE_PIN, OUTPUT);
  digitalWrite(ONE_WIRE_PIN, LOW);

  if (bit) {
    delayMicroseconds(10);
    pinMode(ONE_WIRE_PIN, INPUT_PULLUP);
    delayMicroseconds(55);
  }
  else {
    delayMicroseconds(65);
    pinMode(ONE_WIRE_PIN, INPUT_PULLUP);
    delayMicroseconds(5);
  }
}

bool readBit() {

  bool bit;

  pinMode(ONE_WIRE_PIN, OUTPUT);
  digitalWrite(ONE_WIRE_PIN, LOW);
  delayMicroseconds(3);

  pinMode(ONE_WIRE_PIN, INPUT_PULLUP);
  delayMicroseconds(10);

  bit = digitalRead(ONE_WIRE_PIN);

  delayMicroseconds(53);

  return bit;
}

void writeByte(byte data) {

  for (int i = 0; i < 8; i++) {

    writeBit(data & 0x01);
    data >>= 1;

  }

}

byte readByte() {

  byte value = 0;

  for (int i = 0; i < 8; i++) {

    if (readBit()) {
      value |= (1 << i);
    }

  }

  return value;
}

// ================= DS18B20 =================

void startConversion() {

  if (!resetPulse()) {
    Serial.println("Sensor tidak terdeteksi");
    return;
  }

  writeByte(0xCC);   // Skip ROM
  writeByte(0x44);   // Convert T

}

float readTemperature() {

  if (!resetPulse()) {
    Serial.println("Sensor tidak terdeteksi");
    return -1000;
  }

  writeByte(0xCC);
  writeByte(0xBE);   // Read Scratchpad

  for (int i = 0; i < 9; i++) {
    data[i] = readByte();
  }

  int16_t raw = (data[1] << 8) | data[0];
  float temperature = raw / 16.0;

  return temperature;
}



#include<ModbusMaster.h>

#include <Wire.h>
#include "RTClib.h"

RTC_DS3231 rtc;

char daysOfTheWeek[7][12] = {
  "Sunday","Monday","Tuesday","Wednesday","Thursday","Friday","Saturday"
};

float temperature;


//DI => TX  6
//RO => RX 5
//SoftwareSerial mySerial(6, 5);  // RX, TX

int success = 12;
int fail = 11;

float voltage;
float current;
float power;
float energy;

float solar_radiation;

int minutes;
int minutes_prev;

ModbusMaster node;
ModbusMaster node1;

#include <SPI.h>
#include <SD.h>

File dataFile;

float temp = 27;

#include <Wire.h>
#include <LiquidCrystal_I2C.h>

LiquidCrystal_I2C lcd(0x27, 20, 4);
char str[10];

void setup() {
  wdt_enable(WDTO_8S);
  lcd.init();
  lcd.backlight();
  Serial.begin(9600);
  if (!SD.begin(10)) {
    Serial.println("SD card gagal");
    while (1);
  } else {
    Serial.println("SD Card OK");
  }
  //mySerial.begin(9600);
  Serial2.begin(4800);// PYRANOMETER
  Serial3.begin(9600);


  //pinMode(success, OUTPUT);
  //pinMode(fail, OUTPUT);
  // Init in receive mode


  //My slave uses 9600 baud

  
  delay(10);
  Serial.println("starting arduino: ");
  Serial.println("setting up Serial ");
  Serial.println("setting up RS485 port ");
//  slave id
  node.begin(1, Serial2);
  node1.begin(1, Serial3);
//rtc.adjust(DateTime(2026, 3, 9, 20, 0, 0));
//rtc.adjust(DateTime(F(__DATE__), F(__TIME__)));
  
  
  if (!rtc.begin()) {
    Serial.println("Couldn't find RTC");
    while (1);
  }

  if (rtc.lostPower()) {
    Serial.println("RTC lost power, setting time!");
    
    // Set waktu sesuai waktu compile
    rtc.adjust(DateTime(F(__DATE__), F(__TIME__)));
    
    // Atau set manual seperti ini
    // rtc.adjust(DateTime(2026, 3, 9, 15, 30, 0));
  }
  

  //sensors.begin();  

}


void loop() {
  wdt_reset();
  
  int result;
  uint16_t data[10]; // Array to store the read data
  
  // Read holding registers starting from address 0, read 10 registers
  result = node.readHoldingRegisters(0, 10);
  // Check if the read operation was successful
  if (result == node.ku8MBSuccess) {
    // Print each register value
    for (int i = 0; i < 10; i++) {
      data[i] = node.getResponseBuffer(i); // Get the value of each register
      if(i==0){ 
      solar_radiation = data[i];
      /*
      Serial.print("Register ");
      Serial.print(i);
      Serial.print(": ");
      Serial.println(data[i]);
      */
      }
    }
  } else {
      Serial.println("PYRO ERROR");
  }


  int result1;
  uint16_t data1[4];

  result1 = node1.readInputRegisters(0, 4);
  if (result1 == node.ku8MBSuccess){
    for(int i=0; i<4;i++){
      data1[i] = node1.getResponseBuffer(i);
      if (i == 0){
        voltage = float(data1[i])/100;
      }

      if (i == 1){
        current = float(data1[i])/10;
      }

      if (i == 2){
        power = float(data1[i]);
      }

      if (i == 3){
        energy = float(data1[i]/1000);
      }
      /*
      Serial.print("Register ");
      Serial.print(i);
      Serial.print(": ");
      Serial.println(data1[i]);
      */
    }
  } else{
    Serial.println("PZEM ERROR");
  }

  //sensors.requestTemperatures();
  //temperature = sensors.getTempCByIndex(0);
  
  if (!conversionRunning) {

    startConversion();
    lastRequest = millis();
    conversionRunning = true;

  }

  if (conversionRunning && millis() - lastRequest >= 750) {

    temp = readTemperature();
    //Serial.println(temp);
   

    conversionRunning = false;

  }


  
  DateTime now = rtc.now();
  minutes = now.minute();

  String yearStr   = String(now.year());
  String monthStr  = (now.month()  < 10 ? "0" : "") + String(now.month());
  String dayStr    = (now.day()    < 10 ? "0" : "") + String(now.day());
  String hourStr   = (now.hour()   < 10 ? "0" : "") + String(now.hour());
  String minuteStr = (now.minute() < 10 ? "0" : "") + String(now.minute());
  String secondStr = (now.second() < 10 ? "0" : "") + String(now.second());

  String dayOfWeek = daysOfTheWeek[now.dayOfTheWeek()];

  String formattedTime = dayOfWeek + ", " +
                         yearStr + "-" + monthStr + "-" + dayStr + " " +
                         hourStr + ":" + minuteStr + ":" + secondStr;

  

  
  String fileName = dayStr + monthStr + yearStr.substring(2) + ".csv";

  if (minutes != minutes_prev){  
    Serial.print(formattedTime);
    Serial.print(" ");
    Serial.print(solar_radiation);
    Serial.print(" ");
    Serial.print(voltage);
    Serial.print(" ");
    Serial.print(current);
    Serial.print(" ");
    Serial.print(power);
    Serial.print(" ");
    Serial.print(energy);
    Serial.print(" ");
    Serial.print(temp);
    Serial.println();
    // cek apakah file sudah ada
    
    if (!SD.exists(fileName)) {
      Serial.println("File belum ada, membuat file...");

      dataFile = SD.open(fileName, FILE_WRITE);

      if (dataFile) {
        dataFile.println("Time,Solar Rad,Voltage,Current,Power,energy, Temperature"); // header CSV
        dataFile.close();
        Serial.println("Header CSV ditulis");
      }
    }
     else {
        Serial.println("save data");
        dataFile = SD.open(fileName, FILE_WRITE);
        if (dataFile) {
        dataFile.println(hourStr + ":" + minuteStr + ":" + secondStr + ","
        + String(solar_radiation)+ "," 
        + String(voltage) + "," 
        + String(current)+ "," 
        + String(power)+","
        +String(energy)+ "," 
        + String(temp)); 
        dataFile.close();
        }
    }
    
    
  }
  
  // Voltage
  dtostrf(voltage,6,2,str);
  lcd.setCursor(0,0);
  lcd.print("Volt : ");
  lcd.print(str);
  lcd.print(" V   ");

  // Current
  dtostrf(current,6,2,str);
  lcd.setCursor(0,1);
  lcd.print("Curr : ");
  lcd.print(str);
  lcd.print(" A   ");

  // Power
  dtostrf(power,6,2,str);
  lcd.setCursor(0,2);
  lcd.print("Power: ");
  lcd.print(str);
  lcd.print(" W   ");

  // Solar Radiation
  dtostrf(solar_radiation,6,1,str);
  lcd.setCursor(0,3);
  lcd.print("Solar: ");
  lcd.print(str);
  lcd.print(" W/m2");

  
  
  minutes_prev = minutes;
}
