#include <M5Core2.h>
#include <Wire.h>
#include <Preferences.h>
#include <WiFi.h>
#include <EEPROM.h> //Needed to record user settings
#include "ncmb.h"
#include "SparkFun_Qwiic_Scale_NAU7802_Arduino_Library.h" // Click here to get the library: http://librarymanager/All#SparkFun_NAU8702

NAU7802 myScale; //Create instance of the NAU7802 class

#define BUF_SIZE 64
//EEPROM locations to store 4-byte variables
#define LOCATION_CALIBRATION_FACTOR 0 //Float, requires 4 bytes of EEPROM
#define LOCATION_ZERO_OFFSET 10 //Must be more than 4 away from previous spot. Long, requires 4 bytes of EEPROM
#define CALIBRATION_FACTOR 1264.68

Preferences preferences;
char wifi_ssid[33];
char wifi_key[65];
WiFiClient  client;
NCMB ncmb;

bool settingsDetected = false; //Used to prompt user to calibrate their scale

//Create an array to take average of weights. This helps smooth out jitter.
#define AVG_SIZE 3
float avgWeights[AVG_SIZE];
float avgWeight = 0;
byte avgWeightSpot = 0;
unsigned long count_read = 0;
unsigned char buffer[BUF_SIZE]; // buffer array for data recieve over serial port
unsigned char Cnumber[BUF_SIZE];
unsigned long CNumL = 0L;
int count = 0; // counter for buffer array
int Ccount = 0; // counter for CardNumber Global Buffer

void setup()
{
  //M5,Serial初期化
  M5.begin(true, false, true, true); // lcd,sd,serial,i2c
  M5.Lcd.setTextSize(4);
  Serial.begin(9600);
  Serial2.begin(9600, SERIAL_8N1, 13, 14);
  Serial.println("Qwiic Scale Example");
  CNumL = 0L;
  Cnumber[0] = buffer[0] = 0x0;

  Wire.begin();
  Wire.setClock(400000); //通信速度を指定


  //Scale初期化結果
  if (myScale.begin() == false)
  {
    Serial.println("Scale not detected. Please check wiring. Freezing...");
    while (1);
  }
  Serial.println("Scale detected!");

  readSystemSettings(); //EEPROMからzeroOffset,calibrationFactor読み込み

  myScale.calculateZeroOffset(64);

  myScale.setSampleRate(NAU7802_SPS_320); //Increase to max sample rate
  myScale.calibrateAFE(); //Re-cal analog front end when we change gain, sample rate, or channel


  // Wi-Fiアクセスポイント情報取得
  preferences.begin("Wi-Fi", true);
  preferences.getString("ssid", wifi_ssid, sizeof(wifi_ssid));
  preferences.getString("key", wifi_key, sizeof(wifi_key));
  preferences.end();

  // Wi-Fi接続
  Serial.printf("接続中 - %s ", wifi_ssid);
  WiFi.begin(wifi_ssid, wifi_key);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  M5.Lcd.println("Connection completed");


  // ** Please enter NIFCLOUD mobile backend information **
  ncmb.init(
    "bb71a43a3561020d8c715a54c0b8cf11e48e04ac3c932f4ed88276b7245753f3",
    "e4017c0a6ee7989850327d78dc1f63eea293475102eb1bb17ef67462396724c1");

  M5.Lcd.print("Ready!");
}

void loop()
{
  if (Serial2.available()) // if date is comming from softwareserial port ==> data is comming from SoftSerial shield
  {
    while (Serial2.available()) // reading data into char array
    {
      buffer[count++] = Serial2.read(); // writing data into array
      if (count == BUF_SIZE)break;
    }
    for (int i = 0; i < count; i++)
    {
      if ((buffer[i] >= 0x30 && buffer[i] <= 0x39) || (buffer[i] >= 0x41 && buffer[i] <= 0x46))
      {
        Cnumber[Ccount++] = buffer[i];                // 0-9 A-F
      }
    }

    clearBufferArray(); // call clearBufferArray function to clear the storaged data from the array
    count = 0; // set counter of while loop to zero

  } else {
    if (Ccount > 11) {                // 11文字を上限として番号を整形
      Cnumber[Ccount] = 0x0;
      int cnt = 0;
      buffer[cnt++] = '0'; // convert buffer(hex) to unsigned long
      buffer[cnt++] = 'x'; // RFIDはhexで読み取られるためunsignedLongに変換するとタグに記載の番号となる
      for (int i = 2; i < 10; i++)
      {
        buffer[cnt++] = Cnumber[i];
      }
      buffer[cnt] = 0x0;
      CNumL = strtoul((const char *)buffer, NULL, 16); // unsigned long で保存しておく
      Serial.print("cardNumber:");
      Serial.println(CNumL); // １０進数に変換したものを表示

      delay(100);

      readWeight();

      String itemValue1 = String(CNumL);
      String itemValue2 = String(avgWeight);
      String content1 = "{ \"petTagId\":\"" + itemValue1 + "\",\"petWeigth\":\"" + itemValue2 + "\"}";
      String timestamp1 = "1986-02-04T12:34:56.123Z";

      NCMBResponse response1 = ncmb.registerObject("PetRecordClass", content1, timestamp1);

      clearBufferArray(); // buffer clear
      Ccount = 0;
      avgWeight = 0;
    }
  }
  if (M5.BtnA.wasPressed() )
  {
    readSystemSettings();　//EEPROMからzeroOffset,calibrationFactor読み込み
    myScale.calculateZeroOffset(64);
    Serial.println("reset ok");
    M5.Lcd.clear();
    M5.Lcd.setCursor(10, 10);
    M5.Lcd.setTextColor(WHITE);
    M5.Lcd.print("Ready!");
  }
  M5.update();  // ボタン操作の状況を読み込む関数(ボタン操作を行う際は必須)
}


void clearBufferArray() // function to clear buffer array
{
  for (int i = 0; i < count; i++)
  {
    buffer[i] = NULL;
  } // clear all index of array with command NULL
}


void readWeight()
{
  if (myScale.available() == true)
  {
    Serial.println("available ok");
    if (myScale.getWeight() > 20)
    {
      Serial.println("getweight ok");
      while (count_read++ < 15)
      {
        long currentReading = myScale.getReading();
        float currentWeight = myScale.getWeight();

        Serial.print("Reading: ");
        Serial.print(currentReading);
        Serial.print("\tWeight: ");
        Serial.print(currentWeight, 2); //Print 2 decimal places

        avgWeights[avgWeightSpot++] = currentWeight; //weight格納
        if (avgWeightSpot == AVG_SIZE) avgWeightSpot = 0; //(AVG_SIZE)回になったら格納クリア

        delay(100);

        M5.Lcd.setCursor(10, 10);
        M5.Lcd.print(currentWeight, 1);
        delay(100);

        M5.Lcd.clear();
        Serial.println();
      }

      count_read = 0;

      for (int x = 0 ; x < AVG_SIZE ; x++)
        avgWeight += avgWeights[x];
      avgWeight /= (AVG_SIZE*1000);

      Serial.print("\tAvgWeight: ");
      Serial.print(avgWeight, 1); //Print 2 decimal places


      M5.Lcd.setTextColor(YELLOW);
      M5.Lcd.setCursor(10, 10);
      M5.Lcd.print(avgWeight, 1);

      
      recordSystemSettings(); //Commit these values to EEPROM
    }
  }
}


//Record the current system settings to EEPROM
void recordSystemSettings(void)
{
  //Get various values from the library and commit them to NVM
  //EEPROM.put(LOCATION_CALIBRATION_FACTOR, myScale.getCalibrationFactor());
  EEPROM.put(LOCATION_ZERO_OFFSET, myScale.getZeroOffset());
}

//Reads the current system settings from EEPROM
//If anything looks weird, reset setting to default value
void readSystemSettings(void)
{
  float settingCalibrationFactor; //Value used to convert the load cell reading to lbs or kg
  long settingZeroOffset; //Zero value that is found when scale is tared

  //Look up the calibration factor
  EEPROM.get(LOCATION_CALIBRATION_FACTOR, settingCalibrationFactor);
  if (true) //settingCalibrationFactor == 0xFFFFFFFF
  {
    settingCalibrationFactor = CALIBRATION_FACTOR;
    EEPROM.put(LOCATION_CALIBRATION_FACTOR, settingCalibrationFactor);
  }

  //Look up the zero tare point
  EEPROM.get(LOCATION_ZERO_OFFSET, settingZeroOffset);
  if (settingZeroOffset == 0xFFFFFFFF)
  {
    settingZeroOffset = 1000L; //Default to 1000 so we don't get inf
    EEPROM.put(LOCATION_ZERO_OFFSET, settingZeroOffset);
  }

  //Pass these values to the library
  myScale.setCalibrationFactor(settingCalibrationFactor);
  myScale.setZeroOffset(settingZeroOffset);

  settingsDetected = true; //Assume for the moment that there are good cal values
  if (settingCalibrationFactor < 0.1 || settingZeroOffset == 1000)
    settingsDetected = false; //Defaults detected. Prompt user to cal scale.
}
