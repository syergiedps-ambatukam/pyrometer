#include<avr/wdt.h> /* Header for watchdog timers in AVR */
String serialBuffer = "";


#define ONE_WIRE_PIN 7

// ================= LOW LEVEL =================
void pinLow() {
  pinMode(ONE_WIRE_PIN, OUTPUT);
  digitalWrite(ONE_WIRE_PIN, LOW);
}

void pinRelease() {
  pinMode(ONE_WIRE_PIN, INPUT_PULLUP);
}

// ================= RESET =================
bool onewire_reset() {
  pinLow();
  delayMicroseconds(480);

  pinRelease();
  delayMicroseconds(70);

  bool presence = !digitalRead(ONE_WIRE_PIN); // harus LOW

  delayMicroseconds(410);
  return presence;
}

// ================= WRITE BIT =================
void onewire_write_bit(bool bit) {
  pinLow();
  if (bit) {
    delayMicroseconds(6);
    pinRelease();
    delayMicroseconds(64);
  } else {
    delayMicroseconds(60);
    pinRelease();
    delayMicroseconds(10);
  }
}

// ================= READ BIT =================
bool onewire_read_bit() {
  pinLow();
  delayMicroseconds(6);

  pinRelease();
  delayMicroseconds(9);

  bool bit = digitalRead(ONE_WIRE_PIN);

  delayMicroseconds(55);
  return bit;
}

// ================= BYTE =================
void onewire_write_byte(byte data) {
  for (int i = 0; i < 8; i++) {
    onewire_write_bit(data & 0x01);
    data >>= 1;
  }
}

byte onewire_read_byte() {
  byte data = 0;
  for (int i = 0; i < 8; i++) {
    if (onewire_read_bit()) {
      data |= (1 << i);
    }
  }
  return data;
}




#include<ModbusMaster.h>

#include <Wire.h>
#include "RTClib.h"

RTC_DS3231 rtc;

char daysOfTheWeek[7][12] = {
  "Sunday","Monday","Tuesday","Wednesday","Thursday","Friday","Saturday"
};

float temperature;

unsigned long pzem_time;
unsigned long pzem_time_prev;


//DI => TX  6
//RO => RX 5
//SoftwareSerial mySerial(6, 5);  // RX, TX


float voltage;
float current;
float power;
float energy;
//float power_scaleup;

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
  wdt_enable(WDTO_4S);
  lcd.init();
  lcd.backlight();

  if (!SD.begin(10)) {
    lcd.setCursor(0,1);
    lcd.print("ERROR, CHECK SD CARD");
    
    Serial.println("SD card gagal");
    while (1);
  } else {
    Serial.println("SD Card OK");
  }
  //mySerial.begin(9600);
  Serial2.begin(9600);
  Serial3.begin(4800);// PYRANOMETER

  // Init in receive mode


  //My slave uses 9600 baud

  Serial.begin(9600);
  delay(10);
  Serial.println("starting arduino: ");
  Serial.println("setting up Serial ");
  Serial.println("setting up RS485 port ");
//  slave id
  node1.begin(1, Serial2);
  node.begin(1, Serial3);


  if (!rtc.begin()) {
    Serial.println("Couldn't find RTC");
    lcd.setCursor(0,1);
    lcd.print("ERROR, CHECK RTC");
    
    while (1);
  }

  
  if (rtc.lostPower()) {
    Serial.println("RTC lost power, setting time!");
    
    // Set waktu sesuai waktu compile
    rtc.adjust(DateTime(F(__DATE__), F(__TIME__)));
    
    // Atau set manual seperti ini
    // rtc.adjust(DateTime(2026, 3, 9, 15, 30, 0));
  }


}

void parseTime(String data) {

  int h,m,s,d,mo,y;

  sscanf(data.c_str(), "%d:%d:%d,%d:%d:%d",
         &h,&m,&s,&d,&mo,&y);

  Serial.print("Jam   : "); Serial.println(h);
  Serial.print("Menit : "); Serial.println(m);
  Serial.print("Detik : "); Serial.println(s);
  Serial.print("Tanggal: "); Serial.println(d);
  Serial.print("Bulan : "); Serial.println(mo);
  Serial.print("Tahun : "); Serial.println(y);


  rtc.adjust(DateTime(y, mo, d, h, m, s));

  Serial.println("RTC Updated");
}


void loop() {
  wdt_reset();

  if (!onewire_reset()) {
    Serial.println("Sensor tidak terdeteksi");
    delay(1000);
    return;
  }

  // Skip ROM (asumsi 1 sensor)
  onewire_write_byte(0xCC);

  // Convert T
  onewire_write_byte(0x44);

  delay(750); // waktu konversi (12-bit)

  // Reset lagi
  if (!onewire_reset()) {
    Serial.println("Sensor hilang");
    delay(1000);
    return;
  }

  onewire_write_byte(0xCC);
  onewire_write_byte(0xBE); // Read Scratchpad

  byte temp_l = onewire_read_byte();
  byte temp_h = onewire_read_byte();

  int16_t raw = (temp_h << 8) | temp_l;

  temp = raw / 16.0;

  

  while (Serial.available()) {
    char c = Serial.read();

    if (c == '\n') {   // akhir data
      parseTime(serialBuffer);
      serialBuffer = "";
    } else {
      serialBuffer += c;
    }
  }

  
  int result;
  uint16_t data[10]; // Array to store the read data
  
  //while (Serial3.available()) Serial3.read();

  //delay(1); // kasih jeda sebelum request
  
  // Read holding registers starting from address 0, read 10 registers
  result = node.readHoldingRegisters(0, 1);
  // Check if the read operation was successful
  if (result == node.ku8MBSuccess) {
    // Print each register value
    for (int i = 0; i < 10; i++) {
      data[i] = node.getResponseBuffer(i); // Get the value of each register
      if(i==0){ 
      solar_radiation = data[i];
      //Serial.print("Register ");
      //Serial.print(i);
      //Serial.print(": ");
      //Serial.println(data[i]);
      //digitalWrite(success, HIGH);
      //digitalWrite(fail, LOW);

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
        // power = float(data1[i])/100;
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

      pzem_time_prev = millis();
    }
  } else{
    Serial.println("PZEM ERROR");


  }
  pzem_time = millis() - pzem_time_prev;
  if (pzem_time > 2500){
      voltage = 0;
      current = 0;
      power = 0; 
  }

  //sensors.requestTemperatures();
  //temperature = sensors.getTempCByIndex(0);


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

  


  String fileName = monthStr + dayStr + yearStr.substring(2) + ".csv";


 



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
    ///*
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
    
    //*/
  }

  // Voltage
  dtostrf(voltage,6,2,str);
  lcd.setCursor(0,1);
  lcd.print("V:");
  lcd.print(str);
  //lcd.print("V");

  // Current
  dtostrf(current,6,2,str);
  lcd.setCursor(0,2);
  lcd.print("I:");
  lcd.print(str);
  //lcd.print("A");

  // Power
  dtostrf(power,6,2,str);
  lcd.setCursor(0,3);
  lcd.print("P:");
  lcd.print(str);
  //lcd.print("W");

  // Solar Radiation
  dtostrf(solar_radiation,6,1,str);
  lcd.setCursor(10,2);
  lcd.print("G:");
  lcd.print(str);
  //lcd.print("W/m2");

  //temperature:
  dtostrf(temp,6,2,str);
  lcd.setCursor(10,1);
  lcd.print("T:");
  lcd.print(str);
  lcd.setCursor(18,1);
  lcd.print("  ");
  //lcd.print("C");

  //time:
  lcd.setCursor(0,0);
  if(now.day()<10) lcd.print("0");
  lcd.print(now.day());
  lcd.setCursor(2,0);
  lcd.print("/");
  lcd.setCursor(3,0);
  if(now.month()<10) lcd.print("0");
  lcd.print(now.month());
  lcd.setCursor(5,0);
  lcd.print("/");
  lcd.setCursor(6,0);
  lcd.print(now.year());
  lcd.setCursor(11,0);
  if(now.hour()<10) lcd.print("0");
  lcd.print(now.hour());
  lcd.setCursor(13,0);
  lcd.print(":");
  lcd.setCursor(14,0);
  if(now.minute()<10) lcd.print("0");
  lcd.print(now.minute());
  lcd.setCursor(16,0);
  lcd.print(":");
  lcd.setCursor(17,0);
  if(now.second()<10) lcd.print("0");
  lcd.print(now.second());

  delay(10);
  
  minutes_prev = minutes;
}
