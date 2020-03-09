
//#define USE_LCDDISPLAY  1
#define FIXED_IDS_ONLY 1   //uses only the fixed IDs specified in globals.h (IDLookup array), anything else will be ignored
//#define SHOWDEGUGINFO 1

#include <EEPROM.h>
#ifdef USE_LCDDISPLAY  
   #include <Wire.h>
#endif
#include <SPI.h>



#include "globals.h"

#include "CC1101.h"
#ifdef USE_LCDDISPLAY  
   #include "Display.h"
#endif
#include "ToyotaRead.h"




int DecodeBitArray()
{
  //convert 1s and 0s array to byte array
  int i;
  int n = 0;
  byte b = 0;

  RXByteCount = 0;
  for (i = 0; i < BitCount; i++)
  {
    b = b << 1;
    b = b + IncomingBits[i];
    n++;
    if (n == 8)
    {
      RXBytes[RXByteCount] = b;
      RXByteCount++;
      n = 0;
      b = 0;
    }

  }
  return (RXByteCount);


}

void setup() {

  byte resp;
  unsigned int t;
  int LEDState = LOW;
  int i;
  int mcount;

  //SPI CC1101 chip select set up
  pinMode(CC1101_CS, OUTPUT);
  digitalWrite(CC1101_CS, HIGH);


  Serial.begin(115200);



  pinMode(LED_RX, OUTPUT);
  pinMode(RXPin, INPUT);


  SPI.begin();
  //initialise the CC1101
  CC1101_reset();

  delay(2000);

  Serial.println("Starting...");



  setIdleState();
  digitalWrite(LED_RX, LED_OFF);

  resp = readStatusReg(CC1101_PARTNUM);
  Serial.print(F("Part no: "));
  Serial.println(resp, HEX);

  resp = readStatusReg(CC1101_VERSION);
  Serial.print(F("Version: "));
  Serial.println(resp, HEX);

#ifdef USE_LCDDISPLAY 
  #if USE_ADAFRUIT
    if (!display.begin(SSD1306_EXTERNALVCC, I2C_ADDRESS)) {
      Serial.println(F("SSD1306 allocation failed"));
      for (;;); // Don't proceed, loop forever
    }
  #else
    Wire.begin();
    Wire.setClock(400000L);
    display.begin(&Adafruit128x64, I2C_ADDRESS);
    display.setFont(Adafruit5x7);
  
  #endif
  Serial.println(F("SSD1306 initialised OK"));
#endif






  digitalWrite(LED_RX, LED_ON);
  LEDState = HIGH;

  pinMode(DEBUGPIN, OUTPUT);

#ifndef USE_PROGMEMCRC
  CalulateTable_CRC8();
#endif

  // Clear the buffer
#ifdef USE_LCDDISPLAY 
  #ifdef USE_ADAFRUIT
    display.clearDisplay();
    display.display();
  #else
    display.clear();
  #endif
#endif

  InitTPMS();


  digitalWrite(LED_RX, LED_OFF);

  setRxState();
}

void loop() {
  // put your main code here, to run repeatedly:
  int i;
  static long lastts = millis();
  float diff;
  int RXBitCount = 0;
  int ByteCount = 0;
  byte crcResult;


  #ifdef USE_LCDDISPLAY
     TPMS_Changed = Check_TPMS_Timeouts();
  #endif

  InitDataBuffer();

  //wait for carrier status to go low
  while (GetCarrierStatus() == true)
  {
  }

  //wait for carrier status to go high  looking for rising edge
  while (GetCarrierStatus() == false)
  {
  }
  
  ReceiveMessage();

  #ifdef USE_LCDDISPLAY
    if (TPMS_Changed)
    {
      UpdateDisplay();
      TPMS_Changed = false;
    }
  #endif



}
