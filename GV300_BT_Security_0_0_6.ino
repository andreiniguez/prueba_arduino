//------------------------------------------------------------------------------------------------
//
// Author:                  Juan Carlos Simbaña
// Creation Date:           2019/03/29
// Modification Dates:      
//
//------------------------------------------------------------------------------------------------
//
// Description:
//
//    This program works as a security interface between the GV300 unit and the bluetooth module.
//
//------------------------------------------------------------------------------------------------

#include <SoftwareSerial.h>
#include "MD5.h"

//App Info
#define APP_INFO                              "GV300-Bluetooth Security Interface"

//Firmware Config
#define FIRMWARE_VERSION                      "0.0.6"

//Serial Config
#define SERIAL_SPEED                          9600

//Output Serial Password
#define ACCESS_COMMAND                        "AT+LWHS" //AT+LWHS$
#define MIN_RND                               10000
#define MAX_RND                               1000000000

//Device Imei
#define IMEI_MESSAGE_HEADER                   "+RESP:LWID"
#define GPS_MESSAGE_HEADER1                   "+RESP:GTUDT"
#define GPS_MESSAGE_HEADER2                   "+RESP:GTFRI"
#define SEND_IMEI_ELAPSED_TIME                10000

//Serial Input Variables
String bluetoothInputString =                 "";
String locatorInputString =                   "";

//Serial Output Variables
SoftwareSerial outputSerial(11, 10);          //TX RX

//Cyphering Variables
long secretKey1 =                             169728543;
long secretKey2 =                             251439678;
long secretKey3 =                             394586721;
int accessLevel =                             4; //0; //This must be on zero for enabling the MD5 security
long challengeQuestion =                      0;

//Device Imei
String deviceImei =                           "00";
unsigned long lastSendImeiTime =              0;

//----------------------------------------------------

void setup() {
  outputSerial.begin(SERIAL_SPEED);
  while (!outputSerial) {;}

  outputSerial.write(APP_INFO);
  outputSerial.write(" ");
  outputSerial.write(FIRMWARE_VERSION);
  outputSerial.write("\n"); 

  Serial.begin(SERIAL_SPEED);
  while (!Serial) {;}
  
  Serial.write(APP_INFO);
  Serial.write(" ");
  Serial.write(FIRMWARE_VERSION);
  Serial.write("\n"); 

  deviceImei.reserve(400);
  bluetoothInputString.reserve(200);
  locatorInputString.reserve(200);
  
  randomSeed(analogRead(0));

  lastSendImeiTime = millis();
}

//----------------------------------------------------

void loop() {
  while(outputSerial.available()) 
    BluetoothCommandsEvent();       

  if(deviceImei == "00") {
    while(Serial.available()) 
      ImeiCatchEvent();
  }
  else {
    if(accessLevel == 4)
      while(Serial.available())
        BridgeEvent();
    
    if(millis() - lastSendImeiTime > SEND_IMEI_ELAPSED_TIME) {
      lastSendImeiTime = millis();

      SendImeiToBluetooth();
    }
  }    
}

//----------------------------------------------------

void ImeiCatchEvent() {  
  char char2 = (char)Serial.read();

  locatorInputString += char2;

  if(char2 == '$')  {   
    String inputString = locatorInputString.substring(0, locatorInputString.length() - 1);

    if(inputString.indexOf(GPS_MESSAGE_HEADER1) > 0)
      deviceImei = GetToken(inputString, ',', 5);      
    else if(inputString.indexOf(GPS_MESSAGE_HEADER2) > 0)
      deviceImei = GetToken(inputString, ',', 2);            
  
    locatorInputString = "";        
  }
}

void BluetoothCommandsEvent() {
  char char2 = (char)outputSerial.read();

  bluetoothInputString += char2;

  if(char2 == '$') { 
    String inputString = bluetoothInputString.substring(0, bluetoothInputString.length() - 1);
    String challengeResponse = "";
    
    switch(accessLevel) {

      case 0:

        if(inputString.indexOf(ACCESS_COMMAND) > 0) {       
          accessLevel = 1;      
          
          challengeQuestion = random(MIN_RND, MAX_RND);   
          
          outputSerial.println(challengeQuestion);
        }
        else {
          accessLevel = 0;
          
          outputSerial.println("LOCKED");
        }
        
        break;
        
      case 1:

        challengeResponse = GetMD5(String(challengeQuestion + secretKey1));

        if (inputString.indexOf(challengeResponse) > 0) {
          accessLevel = 2;
  
          challengeQuestion = random(MIN_RND, MAX_RND);
  
          outputSerial.println(challengeQuestion);
        }
        else {
          accessLevel = 0;
  
          outputSerial.println("LOCKED");
        }

        break;

      case 2:

        challengeResponse = GetMD5(String(challengeQuestion + secretKey2));

        if (inputString.indexOf(challengeResponse) > 0) {
          accessLevel = 3;
  
          challengeQuestion = random(MIN_RND, MAX_RND);
  
          outputSerial.println(challengeQuestion);
        }
        else {
          accessLevel = 0;
          
          outputSerial.println("LOCKED");
        }

        break;
        
      case 3:

        challengeResponse = GetMD5(String(challengeQuestion + secretKey3));

        if (inputString.indexOf(challengeResponse) > 0) {
          accessLevel = 4;
   
          outputSerial.println("UNLOCKED");
        }
        else {
          accessLevel = 0;
          
          outputSerial.println("LOCKED");
        }

        break;
    }
        
    bluetoothInputString = "";        
  }             
}

void BridgeEvent() {  
  outputSerial.write(Serial.read());          
}

//----------------------------------------------------

void SendImeiToBluetooth () { 
  outputSerial.print(IMEI_MESSAGE_HEADER);
  outputSerial.print(":");
  outputSerial.print(deviceImei);
  outputSerial.println("$");
}

String GetMD5(String rawString)
{
  char rawCharArray[50];
  
  rawString.toCharArray(rawCharArray, 50);

  unsigned char* hash = MD5::make_hash(rawCharArray);
  
  char *md5String = MD5::make_digest(hash, 16);

  free(hash);
    
  String returnMd5String = md5String;
  
  free(md5String);  

  return returnMd5String;
}

String GetToken(String text, char tokenSeparator, int tokenIndex)
{
    int textLength = text.length() - 1;
    int tokenCounter = 0;
    int tokenBeginCharIndex = 0;
    int tokenEndCharIndex = -1;

    for (int charIndex = 0; charIndex <= textLength && tokenCounter <= tokenIndex; charIndex++) {
        if (locatorInputString.charAt(charIndex) == tokenSeparator || charIndex == textLength) {
            tokenCounter++;            
            tokenBeginCharIndex = tokenEndCharIndex + 1;
            tokenEndCharIndex = (charIndex == textLength) ? charIndex + 1 : charIndex;
        }
    }
    
    return tokenCounter > tokenIndex ? text.substring(tokenBeginCharIndex, tokenEndCharIndex) : "";
}
