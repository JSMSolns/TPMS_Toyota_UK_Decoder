


void ClearTPMSData(int i)
{
  if (i > 4)
    return;

  TPMS[i].TPMS_ID = 0;
  TPMS[i].lastupdated = 0;

}

void PulseDebugPin(int width_us)
{
  digitalWrite(DEBUGPIN, HIGH);
  delayMicroseconds(width_us);
  digitalWrite(DEBUGPIN, LOW);
}


int GetPreferredIndex(unsigned long ID)
{
  int i;

  for (i = 0; i  < (sizeof(IDLookup) / sizeof(IDLookup[0])); i++)
  {
    if (IDLookup[i] == ID)
    {
      return (i);
    }

  }
  return (-1);
}



void PrintTimings(byte StartPoint, byte Count)
{
  byte i;

  Serial.print("Initial state = ");
  Serial.println((FirstEdgeState ? "HIGH" : "LOW"));
  
  for (i = 0; i < Count; i++)
  {
    Serial.print(Timings[StartPoint + i]);
    Serial.print(F(","));
  }
  Serial.println(F(""));
  //    for (i = 0;i<Count;i++)
  //    {
  //          Serial.print(BitTimings[StartPoint + i]);
  //          Serial.print(",");
  //    }
  //    Serial.println("");


}
void PrintBits(byte StartPoint, byte Count)
{
  byte i;
  for (i = 0; i < Count; i++)
  {
    Serial.print(IncomingBits[StartPoint + i]);
    Serial.print(F(","));
  }
  Serial.println(F(""));
}

void PrintData(byte Count)
{
  byte i;
  byte hexdata;
  for (i = 0; i < Count; i++)
  {
    Serial.print(IncomingBits[i]);
    hexdata = (hexdata << 1) + IncomingBits[i];
    if ((i + 1) % 8 == 0)
    {
      Serial.print(F(" ["));
      Serial.print(hexdata, HEX);
      Serial.print(F("] "));
      hexdata = 0;
    }
  }
  Serial.println(F(""));
}



void InitTPMS()
{
  int i;

  for (i = 0; i < 4; i++)
  {
    ClearTPMSData(i);
  }
  #ifdef USE_LCDDISPLAY 
     UpdateDisplay();
  #endif

  #ifdef FIXED_IDS_ONLY
      //pre-set the IDs to their fixed values...
      for (i = 0; i < 4; i++)
      {
        TPMS[i].TPMS_ID = IDLookup[i];
      }


  #endif

}

void UpdateTPMSData(int index, unsigned long ID, unsigned int status, float Temperature, float Pressure)
{

  if (index >= 4)
    return;

  TPMS[index].TPMS_ID = ID;
  TPMS[index].TPMS_Status = status;
  TPMS[index].lastupdated = millis();
  TPMS[index].TPMS_Temperature = Temperature;
  TPMS[index].TPMS_Pressure = Pressure;
}

void PrintTPMSData(int index)
{

  if (index >= 4)
    return;

  Serial.print(F("ID: "));
  Serial.print(TPMS[index].TPMS_ID, HEX);
  Serial.print(F(",   Wheel: "));
  switch(index)
  {
    case 0:  Serial.print(F("FL")); break;
    case 1:  Serial.print(F("FR")); break;
    case 2:  Serial.print(F("RL")); break;
    case 3:  Serial.print(F("RR")); break;

  }
  Serial.print(F(",   Status: "));
  Serial.print(TPMS[index].TPMS_Status);
  Serial.print(F(",   Temperature (degC): "));
  Serial.print(TPMS[index].TPMS_Temperature, 1);
  Serial.print(F(",   Tyre Pressure (kpa): "));
  Serial.print(TPMS[index].TPMS_Pressure * 6.895, 2);
  Serial.print(F(",   Tyre Pressure (psi): "));
  Serial.print(TPMS[index].TPMS_Pressure, 2);
  Serial.println(F(""));

}


boolean Check_TPMS_Timeouts()
{
   byte i;
   boolean ret = false;
    
    //clear any data not updated in the last 5 minutes
  for (i = 0; i < 4; i++)
  {


    if ((TPMS[i].TPMS_ID != 0) && (millis() - TPMS[i].lastupdated > TPMS_TIMEOUT))
    {

      ClearTPMSData(i);
      ret = true;
    }

  }
  return(ret);
}

void DecodeTPMS_PMVC010()
{
  int i;
  unsigned long id = 0;
  unsigned int status, pressure1, pressure2, temp;
  float realpressure;
  float realtemp;
  bool IDFound = false;
  int prefindex;

  for (i = 0; i <= 3; i++)
  {
    id = id << 8;
    id = id + RXBytes[i];
  }

  // id = (unsigned)RXBytes[0] << 24 | RXBytes[1] << 16 | RXBytes[2] << 8 | RXBytes[3];

  status = (RXBytes[4] & 0x80) | (RXBytes[6] & 0x7F)   ;

  pressure1 = (RXBytes[4] & 0x7F)  << 1 | (RXBytes[5] >> 7);

  temp = (RXBytes[5] & 0x7F) << 1 | RXBytes[6] >> 7;

  pressure2 = RXBytes[7] ^ 0xFF;



  if (pressure1 != pressure2)
  {
    #ifdef SHOWDEBUGINFO
       Serial.println(F("Pressure check mis-match"));
    #endif
    return;
  }

  realpressure = (pressure1 * 0.25) - 7;
  realtemp = temp - 40.0;

//  Serial.print(F("Sensor type: Toyota PMV-C010,   "));
//  Serial.print(F("ID: "));
//  Serial.print(id, HEX);
//  Serial.print(F(",   Status: "));
//  Serial.print(status);
//  Serial.print(F(",   Temperature (degC): "));
//  Serial.print(realtemp, 1);
//  Serial.print(F(",   Tyre Pressure (kpa): "));
//  Serial.print(realpressure * 6.895, 2);
//  Serial.print(F(",   Tyre Pressure (psi): "));
//  Serial.print(realpressure, 2);
//  Serial.println(F(""));


  //update the array of tyres data
  for (i = 0; i < 4; i++)
  { //find a matching ID if it already exists
    if (id == TPMS[i].TPMS_ID)
    {
      UpdateTPMSData(i, id, status, realtemp, realpressure);
      PrintTPMSData(i);
      IDFound = true;
      break;
    }

  }

  //no matching IDs in the array, so see if there is an empty slot to add it into, otherwise, ignore it.
  
  #ifdef FIXED_IDS_ONLY
    if (IDFound == false)
    {
  
      prefindex = GetPreferredIndex(id);
      if (prefindex == -1)
      { //not found a specified index, so use the next available one..
        for (i = 0; i < 4; i++)
        {
          if (TPMS[i].TPMS_ID == 0)
          {
            UpdateTPMSData(i, id, status, realtemp, realpressure);
          }
        }
      }
      else
      { //found a match in the known ID list...
        UpdateTPMSData(prefindex, id, status, realtemp, realpressure);
      }
  
    }
  #endif

  #ifdef SHOWDEGUGINFO
     //Serial.println(F(""));
  #endif

}

void DecodeTPMS()
{
  int i;
  unsigned long id = 0;
  unsigned int status, pressure1, pressure2, temp;
  float realpressure;
  float realtemp;
  bool IDFound = false;
  int prefindex;

  for (i = 0; i < 4; i++)
  {
    id = id << 8;
    id = id + RXBytes[i];

  }

  // id = (unsigned)RXBytes[0] << 24 | RXBytes[1] << 16 | RXBytes[2] << 8 | RXBytes[3];

  status = (RXBytes[4] & 0x80) | (RXBytes[6] & 0x7f); // status bit and 0 filler

  pressure1 = (RXBytes[4] & 0x7f) << 1 | RXBytes[5] >> 7;

  temp = (RXBytes[5] & 0x7f) << 1 | RXBytes[6] >> 7;

  pressure2 = RXBytes[7] ^ 0xff;



  if (pressure1 != pressure2)
  {
    #ifdef SHOWDEBUGINFO
       Serial.println(F("Pressure check mis-match"));
    #endif
    return;
  }

  realpressure = pressure1 * 0.25 - 7.0;
  realtemp = temp - 40.0;

//#ifdef SHOWDEGUGINFO
  Serial.print(F("ID: "));
  Serial.print(id, HEX);
  Serial.print(F("   Status: "));
  Serial.print(status);
  Serial.print(F("   Temperature: "));
  Serial.print(realtemp);
  Serial.print(F("   Tyre Pressure: "));
  Serial.print(realpressure);
  Serial.println(F(""));
//#endif

  //DisplayStatusInfo();

  //update the array of tyres data
  for (i = 0; i < 4; i++)
  { //find a matching ID if it already exists
    if (id == TPMS[i].TPMS_ID)
    {
      UpdateTPMSData(i, id, status, realtemp, realpressure);
      IDFound = true;
      break;
    }

  }

  //no matching IDs in the array, so see if there is an empty slot to add it into, otherwise, ignore it.
  if (IDFound == false)
  {

    prefindex = GetPreferredIndex(id);
    if (prefindex == -1)
    { //not found a specified index, so use the next available one..
      for (i = 0; i < 4; i++)
      {
        if (TPMS[i].TPMS_ID == 0)
        {
          UpdateTPMSData(i, id, status, realtemp, realpressure);
        }
      }
    }
    else
    { //found a match in the known ID list...
      UpdateTPMSData(prefindex, id, status, realtemp, realpressure);
    }

  }


  #ifdef SHOWDEGUGINFO
     Serial.println(F(""));
  #endif


  //UpdateDisplay();
}


#ifndef USE_PROGMEMCRC
  void CalulateTable_CRC8()
  {
    const byte generator = 0x07;
  
    /* iterate over all byte values 0 - 255 */
    for (int divident = 0; divident < 256; divident++)
    {
      byte currByte = (byte)divident;
      /* calculate the CRC-8 value for current byte */
      for (byte bit = 0; bit < 8; bit++)
      {
        if ((currByte & 0x80) != 0)
        {
          currByte <<= 1;
          currByte ^= generator;
        }
        else
        {
          currByte <<= 1;
        }
      }
      /* store CRC value in lookup table */
      crctable[divident] = currByte;
      #ifdef SHOWDEBUGINFO
        Serial.print("0x");
        if (currByte < 16)
           Serial.print("0");
        Serial.print(currByte,HEX);
        Serial.print(", ");
      #endif
    }
  }
#endif

byte Compute_CRC8( int bcount)
{
  byte crc = 0x80;
  int c;
  for (c = 0; c < bcount; c++)
  {
    byte b = RXBytes[c];
    /* XOR-in next input byte */
    byte data = (byte)(b ^ crc);
    /* get current CRC value = remainder */
    #ifdef USE_PROGMEMCRC
        crc = (byte)(pgm_read_byte(&crctable2[data]));
    #else
        crc = (byte)(crctable[data]);
    #endif

  }

  return crc;
}



void ClearRXBuffer()
{
  int i;

  for (i = 0; i < sizeof(RXBytes); i++)
  {
    RXBytes[i] = 0;
  }
}

void EdgeInterrupt()
{
  unsigned long ts = micros();
  unsigned long BitWidth;

  if (TimingsIndex == 0)
  {
    //remember the state of the first entry (all other entries will assume to be toggled from this state)
    FirstEdgeState = !digitalRead(RXPin); //if this is a falling edge, the previous state must have been a high
  }

  if (TimingsIndex == MAXTIMINGS)
  {
    return;
  }


  BitWidth = ts - LastEdgeTime_us;
  if (BitWidth <= 12)  //ignore glitches
  {
    return;
  }
  if (BitWidth > 255)
    BitWidth = 255;

  LastEdgeTime_us = ts;
  //    if ((BitWidth >= 38) && (BitWidth <= 250))
  //    {//ignore out of spec pulses
  Timings[TimingsIndex++] = (byte)BitWidth;
  //    }

  //    digitalWrite(DEBUGPIN,HIGH);
  //    delayMicroseconds(3);
  //    digitalWrite(DEBUGPIN,LOW);




}

bool IsValidSync(byte Width)
{
  if (Width >=  175)
  {
    return (true);
  }
  else
  {
    return (false);
  }
}

bool IsTooShort(unsigned int Width)
{
  if (Width < 35 )
  {
    return (true);
  }
  else
  {
    return (false);
  }
}

bool IsValidShort(byte Width)
{
  if ((Width >= 35) && (Width <= 70))
  {
    return (true);
  }
  else
  {
    return (false);
  }
}

bool IsTooLong(unsigned int Width)
{
  if (Width > 120 )
  {
    return (true);
  }
  else
  {
    return (false);
  }
}

bool IsValidLong(byte Width)
{
  if ((Width >= 70) && (Width <= 120))
  {
    return (true);
  }
  else
  {
    return (false);
  }
}

int ValidateBit()
{
  byte BitWidth = Timings[CheckIndex];
  byte BitWidthNext = Timings[CheckIndex + 1];

  if (IsValidLong(BitWidth))
  {
    return (1);
  }

  if (IsValidShort(BitWidth))
  {
    return (0);
  }

  if (IsValidSync(BitWidth))
  {
    return (2);
  }


  return (-1);

}

void ConvertTimingsToBits()
{
  int i, j, c;

  bool CurrentState = FirstEdgeState;
  bool ChangeState;

  BitCount = 0;


  for (i = 0; i <= TimingsIndex; i++)
  {
    ChangeState = true;
    if (IsValidShort(Timings[i]) )
    {
      IncomingBits[BitCount++] = CurrentState;
    }

    if (IsValidLong(Timings[i]) )
    {
      IncomingBits[BitCount++] = CurrentState;
      IncomingBits[BitCount++] = CurrentState;
    }

    if (IsTooShort(Timings[i]) )
    { //handle bits around the preamble
      if (BitCount >= 30)
      {
        return;
      }
      else
      {
        if (BitCount == 0)
        {
            ChangeState = true;
        }
        else
        {
            ChangeState = false;
        }
      }
    }

    if (IsTooLong(Timings[i]))
    { //handle bits around the preamble
      if (BitCount < 50)
      {
        c = (int)(Timings[i] / (48 ));
        for (j = 1; j <= c; j++)
        {
          IncomingBits[BitCount++] = CurrentState;
        }
      }
      else
      {
        return;
      }
    }

    if (ChangeState)
      CurrentState = !CurrentState;

    if (BitCount >= MAXBITS - 1)
    {
      return;
    }
  }

}



static inline uint8_t bit_at(const uint8_t *bytes, unsigned bit)

{
  return (uint8_t)(bytes[bit >> 3] >> (7 - (bit & 7)) & 1);
}

int FindManchesterStart(const uint8_t *pattern, int pattern_bits_len, int SearchLimit, bool CheckInverse)
{
  int i;

  unsigned int ipos = 0;
  unsigned int ppos = 0; // cursor on init pattern
  int limit = BitCount - 3;

  if (SearchLimit < limit)
  {
    limit = SearchLimit;
  }

  while ((ipos < limit) && (ppos < pattern_bits_len))
  {
    if (IncomingBits[ipos] == bit_at(pattern, ppos))
    {
      ppos++;
      ipos++;
      if (ppos == pattern_bits_len)
      {
        return ipos;
      }
    }
    else
    {
      ipos -= ppos;
      ipos++;
      ppos = 0;
    }
  }

  if (CheckInverse)
  {
    ipos = 0;
    ppos = 0;

    while ((ipos < limit) && (ppos < pattern_bits_len))
    {
      if (IncomingBits[ipos] == (bit_at(pattern, ppos) ^ 1))
      {
        ppos++;
        ipos++;
        if (ppos == pattern_bits_len)
        {
          #ifdef SHOWDEBUGINFO
             Serial.println("Found inverse pattern");
          #endif
          return ipos;
        }
      }
      else
      {
        ipos -= ppos;
        ipos++;
        ppos = 0;
      }
    }
  }

  // Not found
  return -1;

}

int ManchesterDecode(int StartIndex)
{
  int i;
  bool bit1, bit2;
  byte b = 0;
  byte n = 0;

  RXByteCount = 0;
  for (i = StartIndex; i < BitCount - 1; i += 2)
  {
    bit1 = IncomingBits[i];
    bit2 = IncomingBits[i + 1];

    if (bit1 == bit2)
      return RXByteCount;

    b = b << 1;
    b = b + (bit2 == true ? 1 : 0);
    n++;
    if (n == 8)
    {
      RXBytes[RXByteCount] = b;
      RXByteCount++;
      n = 0;
      b = 0;
    }

  }

  return RXByteCount;

}

int DiffManchesterDecode(int StartIndex)
{
  int i;
  bool bit1, bit2;
  byte b = 0;
  byte n = 0;

  RXByteCount = 0;
  for (i = StartIndex; i < BitCount - 1; i += 2)
  {
    bit1 = IncomingBits[i];
    bit2 = IncomingBits[i + 1];


    b = (byte)(b << 1);
    b = (byte)(b + (bit2 == bit1 ? 1 : 0));
    n++;
    if (n == 8)
    {
      RXBytes[RXByteCount] = b;
      RXByteCount++;
      n = 0;
      b = 0;
    }

  }
  //Serial.print(F("RXByteCount: "));
  //Serial.println(RXByteCount);
  return RXByteCount;

}

bool ValidateTimings_PMVC010()
{


  byte BitWidth;
  byte BitWidthNext;
  byte BitWidthNextPlus1;
  byte BitWidthPrevious;
  //unsigned long tmp;
  bool WaitingTrailingZeroEdge = false;
  int ret;
  int ManchesterStartPos = -1;
  //    byte pattern[] = { 0xF0 };
  //    int pattern_bits_len = 7;
  byte pattern[] = { 0xCC, 0xAB };
  //byte pattern2[] = { 0x33, 0x54};
  int pattern_bits_len = 16;
  int MCount;



  if (TimingsIndex < 2 + 72)  //header + valid data (minimum)
  { //not enough in the buffer to consider a valid message
    #ifdef SHOWDEBUGINFO
       Serial.println(F("Insufficient data in buffer"));
    #endif
    return false;
  }

  ConvertTimingsToBits();

  //    ManchesterStartPos = FindManchesterStart(pattern, pattern_bits_len);  //look for 1111000
  //    if (ManchesterStartPos == -1)
  //    {
  //       //possible alternative...
  //       ManchesterStartPos = FindManchesterStart(pattern, pattern_bits_len-1);  //look for 111100
  //    }

  ManchesterStartPos = FindManchesterStart(pattern, pattern_bits_len, 100, true);  //look for 1100 1100 1010 1011  F1 (manchester encoded)
//  if (ManchesterStartPos == -1)
//  {
//    //possible alternative...
//    ManchesterStartPos = FindManchesterStart(pattern2, pattern_bits_len, 100, true);  //look for 0011 0011 0101 0100  inverse of F1 (manchester encoded)
//  }

  ManchesterStartPos = ManchesterStartPos - pattern_bits_len;  //move back to beginning of the bit pattern

  if (ManchesterStartPos < 0)
  {
    #ifdef SHOWDEBUGINFO
      Serial.println(F("Sync header not found"));
      PrintBits(0,BitCount);
    #endif
    return false;
  }

  MCount = DiffManchesterDecode(ManchesterStartPos);

  if (MCount >= 9)
  {
    return true;
  }
  else
  {
    #ifdef SHOWDEBUGINFO
      Serial.print(F("Insufficient Manchester bytes: "));
      Serial.println(MCount);
      Serial.print(F("Message start: "));
      Serial.println(ManchesterStartPos);
    #endif
    return false;
  }


}




void ValidateTimings()
{


  byte BitWidth;
  byte BitWidthNext;
  byte BitWidthNextPlus1;
  byte BitWidthPrevious;
  byte diff = TimingsIndex - CheckIndex;
  //unsigned long tmp;
  bool WaitingTrailingZeroEdge = false;
  int ret;

  StartDataIndex = 0;

  if (diff < 72)
  { //not enough in the buffer to consider a valid message
    Serial.println(F("Insufficient data in buffer"));
    return;
  }

  SyncFound = true;

  while ((diff > 0) && (BitCount < 72))
  { //something in buffer to process...
    diff = TimingsIndex - CheckIndex;

    BitWidth = Timings[CheckIndex];

    if (SyncFound == false)
    {
      if (IsValidSync(BitWidth))
      {
        SyncFound = true;
        BitIndex = 0;
        BitCount = 0;
        WaitingTrailingZeroEdge = false;
        StartDataIndex = CheckIndex + 1;
      }

    }
    else
    {
      ret = ValidateBit();
      switch (ret)
      {
        case -1:
          //invalid bit
          BitIndex = 0;
          BitCount = 0;
          WaitingTrailingZeroEdge = false;
          StartDataIndex = CheckIndex + 1;
          break;

        case 0:
          if (WaitingTrailingZeroEdge)
          {
            //BitTimings[BitIndex] = BitWidth;
            IncomingBits[BitIndex++] = 0;
            BitCount++;
            WaitingTrailingZeroEdge = false;
          }
          else
          {
            WaitingTrailingZeroEdge = true;
          }
          break;

        case 1:
          if (WaitingTrailingZeroEdge)
          { //shouldn't get a long pulse when waiting for the second short pulse (i.e. expecting bit = 0)
            //try to resync from here?
            BitIndex = 0;
            BitCount = 0;
            WaitingTrailingZeroEdge = false;
            CheckIndex--;  //recheck this entry
            StartDataIndex = CheckIndex + 1;
          }
          else
          {
            //BitTimings[BitIndex] = BitWidth;
            IncomingBits[BitIndex++] = 1;
            BitCount++;
          }
          break;

        case 2:
          SyncFound = true;
          BitIndex = 0;
          BitCount = 0;
          WaitingTrailingZeroEdge = false;
          StartDataIndex = CheckIndex + 1;
          break;
      }
    }
    CheckIndex++;
  }


}



void InitDataBuffer()
{
  BitIndex = 0;
  BitCount = 0;
  ValidBlock = false;
  //WaitingTrailingZeroEdge = false;
  WaitingFirstEdge  = true;
  CheckIndex = 0;
  TimingsIndex = 0;
  SyncFound = false;
  //digitalWrite(DEBUGPIN, LOW);

}

void DisplayStatusInfo()
{
  #ifdef SHOWDEBUGINFO
    Serial.print (F("FreqOffset: "));
    Serial.print (FreqOffset);
    Serial.print (F("  DemodLinkQuality: "));
    Serial.print (DemodLinkQuality);
    Serial.print (F("  RSSI: "));
    Serial.println (RSSIvalue);
  #endif
}

void UpdateStatusInfo()
{

  byte RSSI_Offset = 74;

  FreqOffset = readStatusReg(CC1101_FREQEST);
  DemodLinkQuality = readStatusReg(CC1101_LQI);
  RSSI_Read = readStatusReg(CC1101_RSSI);
  if (RSSI_Read >= 128)
  {
    RSSIvalue = (int)((int)(RSSI_Read - 256) /  2) - RSSI_Offset;
  }
  else
  {
    RSSIvalue = (RSSI_Read / 2) - RSSI_Offset;
  }

}

bool Check_PMV_C010()
{
  byte crcResult;

  if (ValidateTimings_PMVC010() == true)
  {
    crcResult = Compute_CRC8(8);  //calculate CRC over 8 bytes

    if (crcResult != RXBytes[8])  //verify CRC with the 9th byte in the array
    {
      //        Serial.print(F("CRC: "));
      //        Serial.println(crcResult, HEX);
      #ifdef SHOWDEGUGINFO
        Serial.println(F("CRC Check failed"));
        PrintTimings(0, TimingsIndex);
        PrintBits(0, TimingsIndex);
      #endif
      //PrintDecodedData();
      return (false);
    }
    else
    {
      //decode the message...
      DecodeTPMS_PMVC010();
      return (true);
    }
  }
  else
  {
    #ifdef SHOWDEGUGINFO
      PrintTimings(0, TimingsIndex);
      PrintBits(0, TimingsIndex);
    #endif
    //PrintDecodedData();

    return (false);
  }

}

int ReceiveMessage()
{

  //Check bytes in FIFO
  int FIFOcount;
  int resp;
  bool StatusUpdated = false;

  //set up timing of edges using interrupts...
  LastEdgeTime_us = micros();
  CD_Width = micros();
  digitalWrite(LED_RX,LED_ON);
  attachInterrupt(digitalPinToInterrupt(RXPin), EdgeInterrupt, CHANGE);
  while (GetCarrierStatus() == true)
  {
    if (StatusUpdated == false)
    {
      UpdateStatusInfo();
      StatusUpdated = true;
    }
  }
  detachInterrupt(digitalPinToInterrupt(RXPin));
  digitalWrite(LED_RX,LED_OFF);
  //digitalWrite(DEBUGPIN,LOW);

  CD_Width = micros() - CD_Width;
  if ((CD_Width >= 7000) && (CD_Width <= 8800))
  {
    

    //Serial.println(F("Checking"));
    
    CheckIndex = 0;
    //ValidateTimings();
    //ConvertTimingsToBits();
    //Serial.println(F("//"));
    if (Check_PMV_C010() == false)
    {
     #ifdef SHOWDEGUGINFO
       
       Serial.print("Timings index = ");
       Serial.println(TimingsIndex);
       Serial.print("CD Width = ");
       Serial.println(CD_Width);
       Serial.print("Bit count = ");
       Serial.println(BitCount);
       PrintTimings(0, TimingsIndex);
       PrintBits(0,BitCount);
    #endif     
    }
    else
    {
      TPMS_Changed = true;  //indicates the display needs to be updated.
    }
    DisplayStatusInfo();
    //Serial.println(F("//"));
    //Serial.println(F(""));
    

    //Serial.println(F("Checking complete"));
   
    return (BitCount);
  }
  else
  {
    return (0);
  }



}
