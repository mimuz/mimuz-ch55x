#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include "include/ch5xx.h"
#include "include/ch5xx_usb.h"
#include "USBconstant.h"
#include "USBhandler.h"
#include "mimuz-ch55x.h"

#include <Arduino.h>

extern __xdata __at (EP0_ADDR) uint8_t  Ep0Buffer[];
extern __xdata __at (EP1_ADDR) uint8_t  Ep1Buffer[];
extern __xdata __at (EP2_ADDR) uint8_t  Ep2Buffer[];

extern uint8_t UsbConfig;

#ifndef MAX_CDC_PACKET_SIZE
#define MAX_CDC_PACKET_SIZE 32
#endif

#ifndef MAX_MIDI_PACKET_SIZE
#define MAX_MIDI_PACKET_SIZE 32
#endif

// CDC
#define LINE_CODEING_SIZE 7
__xdata uint8_t LineCoding[LINE_CODEING_SIZE]={0x00,0xe1,0x00,0x00,0x00,0x00,0x08};   //Initialize for baudrate 57600, 1 stopbit, No parity, eight data bits

volatile __xdata uint8_t USBByteCountEP2 = 0;       // Bytes of received data on USB endpoint
volatile __xdata uint8_t USBBufOutPointEP2 = 0;     // Data pointer for fetching
volatile __xdata uint8_t UpPoint2_Busy  = 0;        // Flag of whether upload pointer is busy
volatile __xdata uint8_t controlLineState = 0;

__xdata uint8_t usbWritePointer = 0;

typedef void( *pTaskFn)( void );

// MIDI
volatile __xdata uint8_t USBByteCountEP3 = 0;       // Represents data received by the USB endpoint
volatile __xdata uint8_t USBBufOutPointEP3 = 0;     // get data pointer
volatile __xdata uint8_t UpPoint3_Busy  = 0;        // Flag of whether upload pointer is busy

void delayMicroseconds(uint16_t us);
void loadSerialNumber();

__xdata uint8_t sendBuffer[4];
__xdata uint8_t rcvBuffer[MAX_MIDI_PACKET_SIZE];
__xdata uint8_t rcvIndex;
__xdata uint8_t rcvLength;

hdlMidiMessage cbNoteOn;
hdlNoteOff cbNoteOff;
hdlMidiMessage cbCtlChange;

void USBInit(){
  rcvIndex = 0;
  rcvLength = 0;
  loadSerialNumber();        //set UID to Serial Desc
  USBDeviceCfg();            //Device mode configuration
  USBDeviceEndPointCfg();    //Endpoint configuration   
  USBDeviceIntCfg();         //Interrupt configuration    
  UEP0_T_LEN = 0;
  UEP1_T_LEN = 0;            //Pre-use send length must be cleared
  UEP2_T_LEN = 0;                                                          
  UEP3_T_LEN = 0;                                                          
  delay(1500);                                                          
}

// CDC
void resetCDCParameters(){
  USBByteCountEP2 = 0;       //Bytes of received data on USB endpoint
  UpPoint2_Busy = 0;
}

void setLineCodingHandler(){
  for(uint8_t i=0;i<((LINE_CODEING_SIZE<=USB_RX_LEN)?LINE_CODEING_SIZE:USB_RX_LEN);i++){
    LineCoding[i] = Ep0Buffer[i];
  }
}

uint16_t getLineCodingHandler(){
  uint16_t returnLen;
  returnLen = LINE_CODEING_SIZE;
  for(uint8_t i=0;i<returnLen;i++){
    Ep0Buffer[i] = LineCoding[i];
  }
  return returnLen;
}

void setControlLineStateHandler(){
  controlLineState = Ep0Buffer[2];

  if(((controlLineState & 0x01) == 0) && (*((__xdata uint32_t *)LineCoding) == 1200)){ //both linecoding and sdcc are little-endian
    pTaskFn tasksArr[1];
    USB_CTRL = 0;
    EA = 0;                             //Disabling all interrupts is required.
    TMOD = 0;
    tasksArr[0] = (pTaskFn)0x3800;
    delayMicroseconds(50000);
    delayMicroseconds(50000);
    (tasksArr[0])();                    //Jump to bootloader code
    while(1);
  }
}

bool USBSerial(){
  bool result = false;
  if(controlLineState > 0){
    result = true;
  }
  return result;
}

void USBSerial_flush(void){
  if(!UpPoint2_Busy && usbWritePointer>0){
    UEP2_T_LEN = usbWritePointer;                                                   
    UEP2_CTRL = UEP2_CTRL & ~ MASK_UEP_T_RES | UEP_T_RES_ACK;            //Respond ACK
    UpPoint2_Busy = 1;
    usbWritePointer = 0;
  }
}

uint8_t USBSerial_write(__data char c){
  uint16_t waitWriteCount;
  if(controlLineState > 0){
    while(true){
      waitWriteCount = 0;
      while(UpPoint2_Busy){//wait for 250ms or give up, on my mac it takes about 256us
        waitWriteCount++;
        delayMicroseconds(5);
        if(waitWriteCount>=50000) return 0;
      }
      if(usbWritePointer<MAX_CDC_PACKET_SIZE){
        Ep2Buffer[MAX_PACKET_SIZE+usbWritePointer] = c;
        usbWritePointer++;
        return 1;
      }else{
        USBSerial_flush();  //go back to first while
      }
    }
  }
  return 0;
}

uint8_t USBSerial_print_n(uint8_t * __xdata buf, __xdata int len){  //3 bytes generic pointer, not using USBSerial_write for a bit efficiency
  uint16_t waitWriteCount;
  if(controlLineState > 0) {
    while(len > 0){
      waitWriteCount = 0;
      while (UpPoint2_Busy){//wait for 250ms or give up, on my mac it takes about 256us
        waitWriteCount++;
        delayMicroseconds(5);   
        if(waitWriteCount>=50000) return 0;
      }
      while(len > 0){
        if(usbWritePointer<MAX_CDC_PACKET_SIZE){
          Ep2Buffer[MAX_PACKET_SIZE+usbWritePointer] = *buf++;
          usbWritePointer++;
          len--;
        }else{
          USBSerial_flush();  //go back to first while
          break;
        }
      }
    }
  }
  return 0;
}

uint8_t USBSerial_available(){
  return USBByteCountEP2;
}

char USBSerial_read(){
  if(USBByteCountEP2==0) return 0;
  char data = Ep2Buffer[USBBufOutPointEP2];
  USBBufOutPointEP2++;
  USBByteCountEP2--;
  if(USBByteCountEP2==0) {
    UEP2_CTRL = UEP2_CTRL & ~ MASK_UEP_R_RES | UEP_R_RES_ACK;
  }
  return data;
}

void USB_EP2_IN(){
  UEP2_T_LEN = 0;                                                    // No data to send anymore
  UEP2_CTRL = UEP2_CTRL & ~ MASK_UEP_T_RES | UEP_T_RES_NAK;           //Respond NAK by default
  UpPoint2_Busy = 0;                                                  //Clear busy flag
}

void USB_EP2_OUT(){
  if(U_TOG_OK){                                                     // Discard unsynchronized packets
    USBByteCountEP2 = USB_RX_LEN;
    USBBufOutPointEP2 = 0;                                             //Reset Data pointer for fetching
    if(USBByteCountEP2){
      UEP2_CTRL = UEP2_CTRL & ~ MASK_UEP_R_RES | UEP_R_RES_NAK;       //Respond NAK after a packet. Let main code change response after handling.      
    }
  }
}

// MIDI
void setHdlNoteOff(hdlNoteOff pfunc){
  cbNoteOff = pfunc;
}

void setHdlNoteOn(hdlMidiMessage pfunc){
  cbNoteOn = pfunc;
}

void setHdlCtlChange(hdlMidiMessage pfunc){
  cbCtlChange = pfunc;
}

void resetMIDIParameters(){
  USBByteCountEP3 = 0;
  UpPoint3_Busy = 0;
  rcvIndex = 0;
  rcvLength = 0;
}

uint8_t USBMIDIAvailable(){
  uint8_t ret = 0;
  if(UsbConfig){
     ret = USBByteCountEP3;    
  }
  return ret;
}

uint8_t checkMidiMessage(uint8_t *pMidi){
  if(((*(pMidi + 1) & 0xf0)== 0x90)&&(*(pMidi + 3) != 0)){
    return 2;
  }else if(((*(pMidi + 1) & 0xf0)== 0x90)&&(*(pMidi + 3) == 0)){
    return 1;
  }else if((*(pMidi + 1) & 0xf0)== 0x80){
    return 1;
  }else if((*(pMidi + 1) & 0xf0)== 0xb0){
    return 3;
  }else{
    return 0;
  }
}

void processMidiMessage(){
  uint8_t cnt, kindmessage;
  uint8_t len = USBMIDIAvailable();
  uint8_t *p;

  if(len > 0){
    memcpy(rcvBuffer,Ep2Buffer+32,USBByteCountEP3);
    rcvIndex = 0;
    rcvLength = USBByteCountEP3;
    UEP3_CTRL = UEP3_CTRL & ~ MASK_UEP_R_RES | UEP_R_RES_ACK;
    USBByteCountEP3 = 0;
    for(cnt=0;cnt<rcvLength;cnt+=4){
      p = (uint8_t *)(rcvBuffer + cnt);
      kindmessage = checkMidiMessage(p);
      if(kindmessage == 1){
        if(cbNoteOff != NULL){
          (*cbNoteOff)(*(p+1)&0x0f,*(p+2)&0x7f);
        }
      }else if(kindmessage == 2){
        if(cbNoteOn != NULL){
          (*cbNoteOn)(*(p+1)&0x0f,*(p+2)&0x7f,*(p+3)&0x7f);
        }
      }else if(kindmessage == 3){
        if(cbCtlChange != NULL){
          (*cbCtlChange)(*(p+1)&0x0f,*(p+2)&0x7f,*(p+3)&0x7f);
        }
      }
    }
  }
}

uint8_t sendMidiMessage(uint8_t *msg, uint8_t size){
  uint16_t waitWriteCount = 0;
  while (UpPoint3_Busy){
    waitWriteCount++;
    delayMicroseconds(10);   
    if(waitWriteCount>=10000){
      return 0;
    }
  }
  if(size == 4){
    memcpy(Ep2Buffer+32+MAX_PACKET_SIZE, msg, size);
    UEP3_T_LEN = size;
    UEP3_CTRL = UEP3_CTRL & ~ MASK_UEP_T_RES | UEP_T_RES_ACK;
    UpPoint3_Busy = 1;
    return 1;
  }
  return 0;
}

void sendNoteOn(uint8_t ch, uint8_t note, uint8_t vel){
  sendBuffer[0] = 0x09;
  sendBuffer[1] = 0x90 | ch;
  sendBuffer[2] = 0x7f & note;
  sendBuffer[3] = 0x7f & vel;
  sendMidiMessage(sendBuffer,4);
}

void sendNoteOff(uint8_t ch, uint8_t note){
  sendBuffer[0] = 0x08;
  sendBuffer[1] = 0x80 | ch;
  sendBuffer[2] = 0x7f & note;
  sendBuffer[3] = 0;
  sendMidiMessage(sendBuffer,4);
}

void sendCtlChange(uint8_t ch, uint8_t num, uint8_t value){
  sendBuffer[0] = 0x0b;
  sendBuffer[1] = 0xb0 | ch;
  sendBuffer[2] = 0x7f & num;
  sendBuffer[3] = 0x7f & value;
  sendMidiMessage(sendBuffer,4);
}

void sendNRPN(uint8_t ch, uint16_t num, uint16_t value){
    const uint8_t msb = 0x7f & (num >> 7);
    const uint8_t lsb = 0x7f & num;
    sendCtlChange(ch, NRPNLSB, lsb);
    sendCtlChange(ch, NRPNMSB, msb);
    uint8_t valMsb = 0x7f & (value >> 7);
    uint8_t valLsb = 0x7f & value;
    sendCtlChange(ch, DataEntryMSB, valMsb);
    sendCtlChange(ch, DataEntryLSB, valLsb);
}

void sendPitchbend(uint8_t ch, uint8_t bendvalue) {
    // Calculate the two 7-bit values for the pitch bend message
    uint16_t value = 8192 + bendvalue - 64; 
    uint8_t lsb = value & 0x7F;  // Lower 7 bits
    uint8_t msb = (value >> 7) & 0x7F;  // Upper 7 bits
    
    // Construct the MIDI message
    uint8_t sendBuffer[4];  // Assuming sendBuffer is not already defined
    sendBuffer[0] = 0xE0 | ch;  // Pitch bend message status byte with channel
    sendBuffer[1] = lsb;   // Pitch bend LSB
    sendBuffer[2] = msb;   // Pitch bend MSB
    sendBuffer[3] = 0x00;  // Dummy byte (not used for pitch bend)
    sendMidiMessage(sendBuffer,4);
}

void USB_EP3_IN(){
  UEP3_T_LEN = 0;                                             //The pre-used sending length must be cleared
  UEP3_CTRL = UEP3_CTRL & ~ MASK_UEP_T_RES | UEP_T_RES_NAK;   //NAK by default
  UpPoint3_Busy = 0;                                          //clear busy flag
}

void USB_EP3_OUT(){
  if ( U_TOG_OK ){                                            //Out-of-sync packets will be dropped
    USBByteCountEP3 = USB_RX_LEN;
    USBBufOutPointEP3 = 0;                                       //Fetch data pointer reset
    UEP3_CTRL = UEP3_CTRL & ~ MASK_UEP_R_RES | UEP_R_RES_NAK; //NAK when a packet of data is received, the main function finishes processing, and the main function modifies the response mode
  }
}
