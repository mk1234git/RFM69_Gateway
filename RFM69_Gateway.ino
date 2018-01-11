// **********************************************************************************************************
// Moteino gateway/base sketch that works with Moteinos equipped with RFM69W/RFM69HW/RFM69CW/RFM69HCW
// This is a basic gateway sketch that receives packets from end node Moteinos, formats them as ASCII strings
//      with the end node [ID] and passes them to Pi/host computer via serial port
//     (ex: "messageFromNode" from node 123 gets passed to serial as "[123] messageFromNode")
// It also listens to serial messages that should be sent to listening end nodes
//     (ex: "123:messageToNode" sends "messageToNode" to node 123)
// Make sure to adjust the settings to match your transceiver settings (frequency, HW etc).
// **********************************************************************************
// Copyright Felix Rusu 2016, http://www.LowPowerLab.com/contact
// **********************************************************************************
// License
// **********************************************************************************
// This program is free software; you can redistribute it 
// and/or modify it under the terms of the GNU General    
// Public License as published by the Free Software       
// Foundation; either version 3 of the License, or        
// (at your option) any later version.                    
//                                                        
// This program is distributed in the hope that it will   
// be useful, but WITHOUT ANY WARRANTY; without even the  
// implied warranty of MERCHANTABILITY or FITNESS FOR A   
// PARTICULAR PURPOSE. See the GNU General Public        
// License for more details.                              
//                                                  
// Licence can be viewed at                               
// http://www.gnu.org/licenses/gpl-3.0.txt
//
// Please maintain this license information along with authorship
// and copyright notices in any redistribution of this code
// **********************************************************************************
#include <RFM69.h>         //get it here: https://github.com/lowpowerlab/rfm69
#include <RFM69_ATC.h>     //get it here: https://github.com/lowpowerlab/RFM69
#include <RFM69_OTA.h>     //get it here: https://github.com/lowpowerlab/RFM69
#include <SPIFlash.h>      //get it here: https://github.com/lowpowerlab/spiflash
#include <SPI.h>           //included with Arduino IDE (www.arduino.cc)

//****************************************************************************************************************
//**** IMPORTANT RADIO SETTINGS - YOU MUST CHANGE/CONFIGURE TO MATCH YOUR HARDWARE TRANSCEIVER CONFIGURATION! ****
//****************************************************************************************************************
#define NODEID          1 //the ID of this node
#define NETWORKID     100 //the network ID of all nodes this node listens/talks to
#define FREQUENCY     RF69_868MHZ //Match this with the version of your Moteino! (others: RF69_433MHZ, RF69_868MHZ)
//#define ENCRYPTKEY    "sampleEncryptKey" //identical 16 characters/bytes on all nodes, not more not less!
#define IS_RFM69HW_HCW  //uncomment only for RFM69HW/HCW! Leave out if you have RFM69W/CW!
#define ACK_TIME       30  // # of ms to wait for an ack packet
//*****************************************************************************************************************************
//#define ENABLE_ATC    //comment out this line to disable AUTO TRANSMISSION CONTROL
#define ATC_RSSI      -75  //target RSSI for RFM69_ATC (recommended > -80)
//*****************************************************************************************************************************
// Serial baud rate must match your Pi/host computer serial port baud rate!
#define SERIAL_EN     //comment out if you don't want any serial verbose output
#define SERIAL_BAUD  19200
//*****************************************************************************************************************************

/* PIN definitions */
#define LED_PIN          9

#define DHT22_DATA_PIN 8
#define DHT22_VCC_PIN  6
#define DHTTYPE DHT22   // DHT 22  (AM2302), AM2321


//#define USE_BME
//#define USE_DHT

#if defined(__AVR_ATmega32U4__)
  #define RFM69_RESET_PIN 18
  #define INT_PIN 7
  #define INT_NUM 4
  #error "need pin definitions"
#endif

#ifdef SERIAL_EN
  #define DEBUG(input)   {Serial.print(input); delay(1);}
  #define DEBUGln(input) {Serial.println(input); delay(1);}
#else
  #define DEBUG(input);
  #define DEBUGln(input);
#endif

#ifdef ENABLE_ATC
  RFM69_ATC radio(10, INT_PIN /*DIO0/INT*/, true /*isRFM69HW*/, INT_NUM /*IRQ Num*/);
#else
  RFM69 radio(10, INT_PIN /*DIO0/INT*/, true /*isRFM69HW*/, INT_NUM /*IRQ Num*/);
#endif

void setup() {

  pinMode(LED_PIN, OUTPUT);
   
  pinMode(RFM69_RESET_PIN, OUTPUT);
  digitalWrite(RFM69_RESET_PIN,HIGH);
  delay(20);
  digitalWrite(RFM69_RESET_PIN,LOW);
  delay(20);

  int i;
  for(i = 0; i < 20; i++)
  {
    if (Serial)
    {
      break;
    }

    delay(100);
   }
  
  Serial.begin(SERIAL_BAUD);
  delay(10);
  radio.initialize(FREQUENCY,NODEID,NETWORKID);
#ifdef IS_RFM69HW_HCW
  radio.setHighPower(); //must include this only for RFM69HW/HCW!
#endif
#ifdef ENCRYPTKEY
  radio.encrypt(ENCRYPTKEY);
#endif
  
#ifdef ENABLE_ATC
  radio.enableAutoPower(ATC_RSSI);
#endif

  delay(3000); //to allow programming
  
  char buff[50];
  sprintf(buff, "\nTransmitting at %d Mhz...", radio.getFrequency()/1000000);
  DEBUGln(buff);
}

byte ackCount=0;
byte inputLen=0;
char input[64];
byte buff[61];
String inputstr;

#define TYPE_TXPOWERCHANGE 2
typedef struct {
  byte type;
  int8_t txPowerChange;
} tAckData;

void loop() {
  if (radio.receiveDone())
  {
    int rssi = radio.RSSI;
    bool ackRequested = radio.ACKRequested();
    int txPowerChange = 0;
#if 1
    Serial.print("id: ");
    Serial.print(radio.SENDERID);

    if (radio.DATALEN > 0)
    {
      Serial.print(", rssi: ");
      Serial.print(radio.RSSI);
  
      Serial.print(", len: ");
      Serial.print(radio.DATALEN);
  
      Serial.print(", data: ");
      for (byte i = 0; i < radio.DATALEN; i++)
      {
        byte data = (byte) radio.DATA[i];
        byte data1 = data >> 4;
        byte data2 = data & 0xF;
       
        Serial.print(data1, HEX); 
        Serial.print(data2, HEX); 
      }
   }
    //CheckForWirelessHEX(radio, flash, false); //non verbose DEBUG
#endif
    if (ackRequested)
    {
        delay(5);
        txPowerChange = -80 - radio.RSSI;
        //print("sendAck, txPowerChange: %d" % txPowerChange)
        //data = struct.pack("Bb", 2, txPowerChange)
        
        tAckData ackData;
        ackData.type = TYPE_TXPOWERCHANGE;
        ackData.txPowerChange = txPowerChange;
      
        radio.sendACK(&ackData, sizeof(ackData));
    }

    if (ackRequested)
    {
        Serial.print(", ACK-sent: 1, txPowerChange: ");
        Serial.print(txPowerChange); 
    }
    DEBUGln();
    Blink(LED_PIN,3);
  }
}

void Blink(byte PIN, int DELAY_MS)
{
  pinMode(PIN, OUTPUT);
  digitalWrite(PIN,HIGH);
  delay(DELAY_MS);
  digitalWrite(PIN,LOW);
}
