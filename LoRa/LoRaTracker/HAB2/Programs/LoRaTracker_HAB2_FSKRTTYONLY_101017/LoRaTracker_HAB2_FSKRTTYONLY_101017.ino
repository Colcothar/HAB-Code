//**************************************************************************************************
// Note:
//
// Make changes to this Program file at your peril
//
// Configuration changes should be made in the HAB2_Settings file not here !
//
//**************************************************************************************************

#define programname "LoRaTracker_HAB2_FSKRTTYONLY_101017"
#define aurthorname "Stuart Robinson"

#include <Arduino.h>
#include <avr/pgmspace.h>

#include "LoRaTracker_HAB2_FSKRTTYONLY_Settings.h"
#include Board_Definition
#include "Program_Definitions.h"

/*
**************************************************************************************************

  LoRaTracker Programs for Arduino

  Copyright of the author Stuart Robinson

  http://www.LoRaTracker.uk

  These programs may be used free of charge for personal, recreational and educational purposes only.

  This program, or parts of it, may not be used for or in connection with any commercial purpose without
  the explicit permission of the author Stuart Robinson.

  The programs are supplied as is, it is up to individual to decide if the programs are suitable for the intended purpose and
  free from errors.

  Location payload is constructed thus;

  PayloadID,Sequence,Time,Lat,Lon,Alt,Sats,SupplyVolts,Temperature,Resets,Config0byte,StatusByte,RunmAhr,hertzoffset,GPSFixTime,Checksum
  0           1      2   3   4   5    6      7            8         9        10         11        12       13         14        15

  To Do:
  
  Changes:

**************************************************************************************************
*/

char ramc_RemoteControlNode;
char ramc_ThisNode;

int ramc_CalibrationOffset;

unsigned long ramc_TrackerMode_Frequency;           //frequencies, and other parameters, are copied from memory into RAM.
byte ramc_TrackerMode_Power;
byte TRStatus = 0;                                   //used to store current status flag bits


byte ramc_Current_TXconfig1;                        //sets the config of whats transmitted etc
byte stripvalue;
byte sats;                                          //either sats in view from GPGSV or sats from NMEA fix sentence
int internal_temperature;


unsigned int ramc_Sleepsecs;                        //seconds for sleep at end of TX routine
unsigned int ramc_WaitGPSFixSeconds;                //in flight mode, default time to wait for a fix
unsigned int ramc_FSKRTTYbaudDelay;                 //dealy used in FSKRTTY routine to give chosen baud rate


float Fence_Check_Lon;                              //used for fence check

float TRLat;                                        //tracker transmitter co-ordinates
float TRLon;
unsigned int TRAlt;

byte ramc_FSKRTTYRegshift;
byte ramc_FSKRTTYpips;
unsigned int ramc_FSKRTTYleadin;
byte ramc_key0;
byte ramc_key1;
byte ramc_key2;
byte ramc_key3;
byte keypress;

unsigned long GPSonTime;
unsigned long GPSFixTime;
boolean GPS_Config_Error; 


#include Board_Definition                            //include previously defined board file
#include Memory_Library                              //include previously defined Memory Library

#include <SPI.h>
#include <LowPower.h>                                //https://github.com/rocketscream/Low-Power

#include <TinyGPS++.h>                               //http://arduiniana.org/libraries/tinygpsplus/
TinyGPSPlus gps;                                     //create the TinyGPS++ object
TinyGPSCustom GNGGAFIXQ(gps, "GNGGA", 5);            //custom sentences used to detect possible switch to GLONASS mode

#ifdef USE_SOFTSERIAL_GPS
#include <NeoSWSerial.h>                             //https://github.com/SlashDevin/NeoSWSerial  
NeoSWSerial GPSserial(GPSRX, GPSTX);                 //this library is more relaible at GPS init than software serial
#endif

#include GPS_Library                                 //include previously defined GPS Library 

#include "Voltage_Temperature.h"
#include "LoRa3.h"
#include "FSK_RTTY3.h"                               //this version of FSK_RTTY has inverted pips.
#include "Binary2.h"



void loop()
{
  Serial.println();
  Serial.println();
  GPSFixTime = 0;
  internal_temperature = (int) read_Temperature();    //read temp just after sleep, when CPU is closest to ambient

  GPSonTime = millis();
  gpsWaitFix(ramc_WaitGPSFixSeconds, SwitchOn, LeaveOn);

#ifdef Use_Test_Location
  TRLat = TestLatitude;
  TRLon = TestLongitude;
  TRAlt = TestAltitude;
#endif

  if (readConfigByte(CheckFence) && (!doFenceCheck()))      //if fence check is enabled and tracker is outside fence
  {
    action_outside_fence();
  }
  else                                                      //either fence check is disabled or tracker is within it
  {
    if (readConfigByte(TXEnable))                           //is TX enabled ?
    {
      do_Transmissions();                                   //yes, so do transmissions
    }
    else
    {
      Serial.println(F("TX off"));
      inside_fence_no_transmit();                           //no, TX is disabled
    }
  }

  sleepSecs(ramc_Sleepsecs);                                //this sleep is used to set overall transmission cycle time

}




void do_Transmissions()
{
  //this is where all the transmisions get sent
  byte index, Count;

  pulseWDI();
  lora_Setup();                                                      //resets then sets up LoRa device

  Setup_LoRaTrackerMode();

  incMemoryULong(addr_SequenceNum);                                  //increment sequence number
  Count = buildHABPacket();
  stripvalue = readConfigByte(AddressStrip);


  if (readConfigByte(FSKRTTYEnable))
  {

    Serial.println(F("Send FSKRTTY"));

    lora_DirectSetup();                                                //set for direct mode
    lora_SetFreq(ramc_TrackerMode_Frequency, ramc_CalibrationOffset);
    Start_FSKRTTY(FSKRTTYRegshift, FSKRTTYleadin, FSKRTTYpips);

    for (index = 1; index <= sync_chars; index++)
    {
      Send_FSKRTTY('$', FSKRTTYbaudDelay);
    }

    for (index = 0; index <= Count; index++)
    {
      Send_FSKRTTY(lora_TXBUFF[index], FSKRTTYbaudDelay);
    }
    Send_FSKRTTY(13, FSKRTTYbaudDelay);                        //finish RTTY with carriage return
    Send_FSKRTTY(10, FSKRTTYbaudDelay);                        //and line feed
    digitalWrite(LED1, LOW);                                   //make sure LED off

    #ifndef ConstantTX  
    lora_TXOFF();                                              //turns off transmit
    #endif
    
  }
}


byte buildHABPacket()                         //expects a char buffer, so this routine will not work without the -permissive setting
{
  //build the long tracker payload
  unsigned int index, j, CRC, resets, runmAhr;
  int volts;
  byte Count,len;
  char LatArray[12], LonArray[12];

  //unsigned long fixtime;
  unsigned long sequence;

  sequence = Memory_ReadULong(addr_SequenceNum);               //sequence number is kept in non-volatile memory so it survives resets
  resets =  Memory_ReadUInt(addr_ResetCount);                  //reset count is kept in non-volatile memory so it survives resets

  Serial.print("Resets ");
  Serial.println(resets);
  Serial.print("Temperature ");
  Serial.println(internal_temperature);

  runmAhr = 0;

  volts = read_SupplyVoltage();

  if (!readConfigByte(GPSHotFix))
  {
    GPSFixTime = 0;                                              //if GPS power save is off (no GPSHotFix), ensure GPS fix time is set to zero
    Serial.println("GPSFixTime set to 0");
  }

  sats = gps.satellites.value();
  dtostrf(TRLat, 7, 5, LatArray);                              //format is dtostrf(FLOAT,WIDTH,PRECISION,BUFFER);
  dtostrf(TRLon, 7, 5, LonArray);                              //converts float to character array
  len = sizeof(lora_TXBUFF);
  memset(lora_TXBUFF, 0, len);                                 //clear array to 0s

  Count = snprintf((char*) lora_TXBUFF,
                   Output_len_max,
                   "$$$$%s,%lu,%02d:%02d:%02d,%s,%s,%d,%d,%d,%d,%d,%d,%d,%d,%d,%lu",
                   Flight_ID,
                   sequence,
                   gps.time.hour(),
                   gps.time.minute(),
                   gps.time.second(),
                   LatArray,
                   LonArray,
                   TRAlt,
                   sats,
                   volts,
                   internal_temperature,
                   resets,
                   ramc_Current_TXconfig1,
                   TRStatus,
                   runmAhr,
                   ramc_CalibrationOffset,
                   GPSFixTime
                  );

  //Count = strlen(lora_TXBUFF);                    //how long is the array ?

  CRC = 0xffff;                                   //start value for CRC16

  for (index = 4; index < Count; index++)         //element 4 is first character after $$ at start
  {
    CRC ^= (((unsigned int)lora_TXBUFF[index]) << 8);
    for (j = 0; j < 8; j++)
    {
      if (CRC & 0x8000)
        CRC = (CRC << 1) ^ 0x1021;
      else
        CRC <<= 1;
    }
  }

  lora_TXBUFF[Count++] = '*';
  lora_TXBUFF[Count++] = Hex((CRC >> 12) & 15);      //add the checksum bytes to the end
  lora_TXBUFF[Count++] = Hex((CRC >> 8) & 15);
  lora_TXBUFF[Count++] = Hex((CRC >> 4) & 15);
  lora_TXBUFF[Count] = Hex(CRC & 15);
  return Count;
}


char Hex(byte lchar)
{
  //used in CRC calculation in buildHABPacket
  char Table[] = "0123456789ABCDEF";
  return Table[lchar];
}


void inside_fence_no_transmit()
{
//null nothing to do  
}


void action_outside_fence()
{
  sleepSecs(outside_fence_Sleep_seconds);                    //goto sleep for a long time
  
}


byte doFenceCheck()                                         //checks to see if GPS location is within an Western and Eastern limit
{
  //there has been a fix, so check fence limits
  Fence_Check_Lon = gps.location.lng();

#ifdef DEBUGNoGPS
  Fence_Check_Lon = TestLongitude;
#endif

#ifdef DEBUG
  Serial.print(Fence_Check_Lon, 6);
  Serial.print(F(" "));
#endif

  if ((Fence_Check_Lon > west_fence) && (Fence_Check_Lon < east_fence))   //approximate to the limits for region 1 ISM band
  {
    Serial.println(F("Inside Fence"));
    return 1;                                               //within the fence
  }
  else
  {
    Serial.println(F("Outside Fence"));
    return 0;
  }
  //outside the fence
}


void printPayload(byte lCount)
{
  byte index;
  for (index = 0; index <= lCount; index++)
  {
    Serial.write(lora_TXBUFF[index]);
  }
}



void printMemoryFrequencies()
{
  //a useful check to see if memory is configured correctly
  unsigned long tempULong;
  tempULong = Memory_ReadULong(addr_TrackerMode_Frequency);
  Serial.print(F("Memory Tracker Frequency "));
  Serial.println(tempULong);

}


void printRAMFrequencies()
{
  Serial.print(F("RAM Tracker Frequency "));
  Serial.println(ramc_TrackerMode_Frequency);
 
  Serial.print(F("RAM Calibration Offset "));
  Serial.println(ramc_CalibrationOffset);
}



void sleepSecs(unsigned int LNumberSleeps)
{
  unsigned int i;

  Serial.print(F("zz "));
  Serial.println(LNumberSleeps);
  Serial.flush();                                      //let print complete
  
  #ifdef USING_SERIALGPS
  GPSserial.end();                                     //we dont want GPS input interfering with sleep
  #endif
  
  digitalWrite(lora_NSS, HIGH);                        //ensure LoRa Device is off

  for (i = 1; i <= LNumberSleeps; i++)
  {
    LowPower.powerDown(SLEEP_1S, ADC_OFF, BOD_OFF);    //sleep 1 second
    pulseWDI();
  }
  
}


void incMemoryULong(unsigned int laddress)
{
  unsigned long val;
  val = Memory_ReadULong(laddress);
  val++;
  Memory_WriteULong(laddress, val);
}


byte readConfigByte(byte bitnum)
{
  return bitRead(ramc_Current_TXconfig1, bitnum);
}



void setStatusByte(byte bitnum, byte bitval)
{
  //program the status byte

  if (bitval == 0)
  {
    bitClear(TRStatus, bitnum);
  }
  else
  {
    bitSet(TRStatus, bitnum);
  }

#ifdef DEBUG
  if (bitval)
  {
    Serial.print(F("Set Status Bit "));
  }
  else
  {
    Serial.print(F("Clear Status Bit "));
  }

  Serial.println(bitnum);
#endif
}


void pulseWDI()
{
  //if the watchdog is fitted it needs a regular pulse top prevent reset
  //togle the WDI pin twice
  digitalWrite(WDI, !digitalRead(WDI));
  delayMicroseconds(1);
  digitalWrite(WDI, !digitalRead(WDI));
}


boolean gpsWaitFix(unsigned long waitSecs, byte StartState, byte LeaveState)
{
  //waits a specified number of seconds for a fix, returns true for good fix
  //StartState when set to 1 will turn GPS on at routine start
  //LeaveState when set to 0 will turn GPS off at routine end, used perhaps when there is no fix

  unsigned long endwaitmS, millistowait, currentmillis;
  pulseWDI();
  byte GPSByte;


  if (StartState == 1)
  {
    GPS_On(DoGPSPowerSwitch);
    GPSonTime = millis();
  }
  else
  {
    GPS_On(NoGPSPowerSwitch);
  }

  #ifdef Check_GPS_Navigation_Model_OK 
  Serial.println(F("Checking Navigation Model"));
  if (!GPS_CheckNavigation())
  {
  //something wrong with GPS, navigation mode not set so reconfigure the GPS
  Serial.println(F("GPS Configuration Error !!!!"));
  GPS_Setup();  
  }
  #endif
  

  Serial.print(F("Wait Fix "));
  Serial.print(waitSecs);
  Serial.println(F(" Secs"));

  currentmillis = millis();
  millistowait = waitSecs * 1000;
  endwaitmS = currentmillis + millistowait;

  while (millis() < endwaitmS)
  {

    do
    {
      GPSByte = GPS_GetByte();
      if (GPSByte != 0xFF)
      {
        gps.encode(GPSByte);
      }
    }
    while (GPSByte != 0xFF);

    if (gps.location.isUpdated() && gps.altitude.isUpdated())
    {
      Serial.println(F("GPS Fix"));
      TRLat = gps.location.lat();
      TRLon = gps.location.lng();
      TRAlt = (unsigned int) gps.altitude.meters();

      //Altitude is used as an unsigned integer, so that the binary payload is as short as possible.
      //However gps.altitude.meters(); can return a negative value which converts to
      //65535 - Altitude, which we dont want. So we will assume any value over 60,000M is zero

      if (TRAlt > 60000)
      {
        TRAlt = 0;
      }

      if (readConfigByte(GPSHotFix))
      {
        GPS_Off(DoGPSPowerSwitch);
        GPSFixTime = (millis() - GPSonTime);
        Serial.print(F("GPS Fix Time "));
        Serial.print(GPSFixTime);
        Serial.println(F("mS"));
      }
      else
      {
        GPS_Off(NoGPSPowerSwitch);
      }

      setStatusByte(GPSFix, 1);
      pulseWDI();
      return true;
    }

#ifdef UBLOX
    if (GNGGAFIXQ.age() < 2000)                     //check to see if GLONASS has gone active
    {
      Serial.println(F("GLONASS !"));
      setStatusByte(GLONASSisoutput, 1);
      GPS_SetGPMode();
      //GPS_SetCyclicMode();
      sleepSecs(1);
    }
    else
    {
      setStatusByte(GLONASSisoutput, 0);            //GLONASS not detected
    }
#endif



  }

  //if here then there has been no fix and a timeout
  setStatusByte(GPSFix, 0);                       //set status bit to flag no fix
  Serial.println(F("No Fix"));

  if (LeaveState == 0)
  {
    //no fix and gpsWaitFix called with gpspower to be turned off on exit
    GPS_Off(DoGPSPowerSwitch);
  }
  else
  {
    //no fix but gpsWaitFix called with gpspower to be left on at exit
    GPS_Off(NoGPSPowerSwitch);
  }

  pulseWDI();
  return false;
}


void readSettingsDefaults()
{
  //To ensure the program routines are as common as possible betweeen transmitter and receiver
  //this program uses constants in RAM copied from Memory (EEPROM or FRAM) in the same way as the transmitter.
  //There are some exceptions, where the local programs need to use a setting unique to the particular
  //receiver.
  Serial.println(F("Config Defaults"));
  ramc_TrackerMode_Frequency = TrackerMode_Frequency;
  ramc_TrackerMode_Power = TrackerMode_Power;

  ramc_ThisNode = ThisNode;

  ramc_Current_TXconfig1 = Default_config1;
  ramc_WaitGPSFixSeconds = WaitGPSFixSeconds;
  ramc_Sleepsecs = Loop_Sleepsecs;

  ramc_FSKRTTYbaudDelay = FSKRTTYbaudDelay;
  ramc_FSKRTTYRegshift = FSKRTTYRegshift;
  ramc_FSKRTTYpips = FSKRTTYpips;
  ramc_FSKRTTYleadin = FSKRTTYleadin;

  ramc_CalibrationOffset = CalibrationOffset;
}


void readSettingsMemory()
{
  //To ensure the program routines are as common as possible betweeen transmitter and receiver
  //this program uses constants in RAM copied from Memory (EEPROM or FRAM) in the same way as the transmitter.
  //There are some exceptions, where the local programs need to use a setting unique to the particular
  //receiver.
  Serial.println(F("Config from Memory"));

  ramc_TrackerMode_Frequency = Memory_ReadULong(addr_TrackerMode_Frequency);
  ramc_TrackerMode_Power = Memory_ReadByte(addr_TrackerMode_Power);

  ramc_ThisNode = Memory_ReadByte(addr_ThisNode);;
 
  ramc_Current_TXconfig1 = Memory_ReadByte(addr_Default_config1);
  ramc_WaitGPSFixSeconds = Memory_ReadUInt(addr_WaitGPSFixSeconds);
  ramc_Sleepsecs = Memory_ReadUInt(addr_Sleepsecs);
 
  ramc_FSKRTTYbaudDelay = Memory_ReadUInt(addr_FSKRTTYbaudDelay);
  ramc_FSKRTTYRegshift = Memory_ReadByte(addr_FSKRTTYRegshift);
  ramc_FSKRTTYpips = Memory_ReadByte(addr_FSKRTTYpips);
  ramc_FSKRTTYleadin = Memory_ReadUInt(addr_FSKRTTYleadin);
  ramc_key0 = Memory_ReadByte(addr_key0);
  ramc_key1 = Memory_ReadByte(addr_key1);
  ramc_key2 = Memory_ReadByte(addr_key2);
  ramc_key3 = Memory_ReadByte(addr_key3);

  ramc_CalibrationOffset = Memory_ReadInt(addr_CalibrationOffset);

  readIDMemory();
}


void writeSettingsMemory()
{
  //To ensure the program routines are as common as possible betweeen transmitter and receiver
  //this program uses constants in RAM copied from Memory (EEPROM or FRAM)

  Serial.println(F("Writing RAM Settings to Memory"));

  Memory_Set(addr_StartConfigData, addr_EndConfigData, 0);          //fill config area with 0
  writeIDMemory();

  Memory_WriteULong(addr_TrackerMode_Frequency, ramc_TrackerMode_Frequency);
  Memory_WriteByte(addr_TrackerMode_Power, ramc_TrackerMode_Power);

  Memory_WriteByte(addr_Default_config1, ramc_Current_TXconfig1);
  
  Memory_WriteByte(addr_ThisNode, ramc_ThisNode);
  
  Memory_WriteByte(addr_Default_config1, ramc_Current_TXconfig1);
  Memory_WriteUInt(addr_WaitGPSFixSeconds, ramc_WaitGPSFixSeconds);
  Memory_WriteUInt(addr_Sleepsecs, ramc_Sleepsecs);
  
  Memory_WriteUInt(addr_FSKRTTYbaudDelay, ramc_FSKRTTYbaudDelay);
  Memory_WriteByte(addr_FSKRTTYRegshift, ramc_FSKRTTYRegshift);
  Memory_WriteByte(addr_FSKRTTYpips, ramc_FSKRTTYpips);
  Memory_WriteUInt(addr_FSKRTTYleadin, ramc_FSKRTTYleadin);
  Memory_WriteByte(addr_key0, ramc_key0);
  Memory_WriteByte(addr_key1, ramc_key1);
  Memory_WriteByte(addr_key2, ramc_key2);
  Memory_WriteByte(addr_key3, ramc_key3);

  Memory_WriteInt(addr_CalibrationOffset, ramc_CalibrationOffset);
}


void writeIDMemory()
{
  unsigned int i, addr;
  byte j;
  j = sizeof(Flight_ID);
  j--;
  addr = addr_FlightID;
  for (i = 0; i <= j; i++)
  {
    Memory_WriteByte(addr, Flight_ID[i]);
    addr++;
  }
}

void readIDMemory()
{
  unsigned int addr;
  byte i;
  byte j = 0;

  addr = addr_FlightID;
  do
  {
    i = Memory_ReadByte(addr);
    Flight_ID[j++] = i;
    addr++;
  }
  while (i != 0) ;
}


void printNodes()
{
  Serial.print(F("ThisNode "));
  Serial.print(ramc_ThisNode);
  Serial.println();
}


void Setup_LoRaTrackerMode()
{
  lora_SetFreq(ramc_TrackerMode_Frequency, ramc_CalibrationOffset);
  lora_Power = ramc_TrackerMode_Power;
}


void display_current_frequency()
{
  float freq_temp;
  freq_temp = lora_GetFreq();
  Serial.print(F("Frequency "));
  Serial.print(freq_temp, 3);
  Serial.println(F("MHz"));
}


void led_Flash(unsigned int flashes, unsigned int delaymS)
{
  //flash LED to show tracker is alive
  unsigned int index;

  for (index = 1; index <= flashes; index++)
  {
    digitalWrite(LED1, HIGH);
    delay(delaymS);
    digitalWrite(LED1, LOW);
    delay(delaymS);
  }
}


//*******************************************************************************************************
// Memory Routines
//*******************************************************************************************************

void do_ClearSavedData()
{
  //clears the whole of memory, normally 1kbyte
  Serial.println(F("Clear Saved Memory"));
  Memory_Set(addr_StartMemory, addr_EndMemory, 0);
}


void Clear_All_Memory()
{
  //clears the whole of memory, normally 1kbyte
  Serial.println(F("Clear All Memory"));
  Memory_Set(addr_StartMemory, addr_EndMemory, 0);
}


void Print_Config_Memory()
{
  //prints the memory used for storing configuration settings
  byte memory_LLoopv1;
  byte memory_LLoopv2;
  unsigned int memory_Laddr = 0;
  byte memory_Ldata;
  //unsigned int CRC;
  Serial.println(F("Config Memory"));
  Serial.print(F("Lcn    0  1  2  3  4  5  6  7  8  9  A  B  C  D  E  F"));
  Serial.println();

  for (memory_LLoopv1 = 0; memory_LLoopv1 <= 15; memory_LLoopv1++)
  {
    Serial.print(F("0x"));
    Serial.print(memory_LLoopv1, HEX);                       //print the register number
    Serial.print(F("0  "));
    for (memory_LLoopv2 = 0; memory_LLoopv2 <= 15; memory_LLoopv2++)
    {
      memory_Ldata = Memory_ReadByte(memory_Laddr);
      if (memory_Ldata < 0x10) {
        Serial.print(F("0"));
      }
      Serial.print(memory_Ldata, HEX);                       //print the register number
      Serial.print(F(" "));
      memory_Laddr++;
    }
    Serial.println();
  }
}


void Print_CRC_Config_Memory()
{
  unsigned int returnedCRC = Memory_CRC(addr_StartConfigData, addr_EndConfigData);
  Serial.print(F("CRC Config "));
  Serial.println(returnedCRC, HEX);
}


unsigned int Print_CRC_Bind_Memory()
{
  unsigned int returnedCRC = Memory_CRC(addr_StartBindData, addr_EndBindData);
  Serial.print(F("Local Bind CRC "));
  Serial.println(returnedCRC, HEX);
  return returnedCRC;
}



//*******************************************************************************************************


void setup()
{
  unsigned long int i;
  unsigned int j;

  pinMode(LED1, OUTPUT);                  //for PCB LED
  pinMode(WDI, OUTPUT);                   //for Watchdog pulse input

  led_Flash(2, 500);

  Serial.begin(38400);                    //Setup Serial console ouput

  Memory_Start();

#ifdef ClearAllMemory
  Clear_All_Memory();
#endif


  Serial.println(F(programname));
  Serial.println(F(aurthorname));

  pinMode(GPSPOWER, OUTPUT);              //in case power switching components are fitted
  GPS_On(DoGPSPowerSwitch);               //this will power the GPSon
  GPSonTime = millis();

#ifdef USING_SERIALGPS
  GPSserial.end();                        //but we dont want soft serial running for now, it interferes with the LoRa device
#endif

  pinMode(lora_NReset, OUTPUT);           //LoRa device reset line
  digitalWrite(lora_NReset, HIGH);

  pinMode (lora_NSS, OUTPUT);             //set the slave select pin as an output:
  digitalWrite(lora_NSS, HIGH);

  SPI.begin();                            //initialize SPI
  SPI.beginTransaction(SPISettings(4000000, MSBFIRST, SPI_MODE0));


#ifdef ClearSavedData
  do_ClearSavedData();
#endif

  Serial.print(F("Config1 "));
  Serial.println(Default_config1, BIN);

  j = Memory_ReadUInt(addr_ResetCount);
  j++;
  Memory_WriteUInt(addr_ResetCount, j);
  Serial.print(F("Resets "));
  Serial.println(j);
  Serial.print(F("Sequence "));
  i = Memory_ReadULong(addr_SequenceNum);
  Serial.println(i);


#ifdef ConfigureDefaults
  readSettingsDefaults();
  writeSettingsMemory();
#endif


#ifdef ConfigureFromMemory
  readSettingsMemory();
#endif

  Print_CRC_Config_Memory();

  ramc_ThisNode = ThisNode;

#ifdef DEBUG
  printNodes();
#endif

  lora_Setup();

  if (!lora_CheckDevice())
  {
    led_Flash(100, 50);                                                //long medium speed flash for Lora device error
    Serial.println(F("LoRa Error!"));
  }

#ifdef DEBUG
  display_current_frequency();
#endif

  ramc_CalibrationOffset = Memory_ReadInt(addr_CalibrationOffset);  //get calibration offset for this tracker
  Serial.print(F("Cal Offset "));
  Serial.println(ramc_CalibrationOffset);

#ifdef DEBUG
  lora_Print();
#endif

  Serial.println();
  print_SupplyVoltage();
  print_Temperature();
  Serial.println();

  j = read_SupplyVoltage();                     //get supply mV
  Write_Int(0, j, lora_TXBUFF);                 //write to first two bytes of buffer
  Write_Byte(2, ramc_Current_TXconfig1, lora_TXBUFF);  //add the current config byte

  Setup_LoRaTrackerMode();
  sleepSecs(1);

  Setup_LoRaTrackerMode();                             //so that check tone is at correct frequency

  GPS_Config_Error = false;                            //make sure GPS error flag is cleared

#ifndef DEBUGNoGPS
  GPS_On(DoGPSPowerSwitch);                                 //GPS should have been on for a while by now, so this is just to start soft serial
  GPSonTime = millis();
  GPS_Setup();                                           //GPS should have had plenty of time to initialise by now

#ifdef UBLOX
  if (!GPS_CheckNavigation())                            //Check that UBLOX GPS is in Navigation model 6
  {
    Serial.println();
    GPS_Config_Error = true;
    setStatusByte(GPSError, 1);
    setStatusByte(UBLOXDynamicModel6Set, 0);
  }
  else
  {
    setStatusByte(UBLOXDynamicModel6Set, 1);
  }
#endif

  if (GPS_Config_Error)
  {
    Serial.println(F("Warning GPS Error !"));
    Serial.println();
    led_Flash(100, 25);                                     //long very rapid flash for GPS error
  }
  else
  {
#ifdef CheckTone
    if (readConfigByte(TXEnable))                           //is TX enabled - needed because of fence limits
    {
      Serial.println(F("Check Tone"));                      //check tone indicates navigation model 6 set (if checktone enabled!)
      lora_Tone(1000, 3000, 5);                             //Transmit an FM tone, 1000hz, 3000ms, 5dBm
    }
#endif
  }

  digitalWrite(LED1, HIGH);
  setStatusByte(NoGPSTestMode, 0);
  GPSonTime = millis();
  while (!gpsWaitFix(5, DontSwitch, LeaveOn))             //while there is no fix
  {

    led_Flash(2, 50);                                     //two short LED flashes to indicate GPS waiting for fix

#ifdef DEBUG
    i = (millis() - GPSonTime) / 1000;
    Serial.print(F("GPS On Time "));
    Serial.print(i);
    Serial.println(F(" Secs"));
#endif
  }
  
#endif

#ifndef DEBUGNoGPS
  GPS_On(DoGPSPowerSwitch);                                                 
  GPS_SetCyclicMode();                                     //set this regardless of whether hot fix mode is enabled
#endif


  lora_Tone(500, 500, 2);                                  //Transmit an FM tone, 500hz, 500ms, 2dBm
  digitalWrite(LED1, LOW);
  sleepSecs(2);                                            //wait for GPS to shut down

#ifdef DEBUGNoGPS
  setStatusByte(NoGPSTestMode, 1);
#endif

}






