#include <string.h>
#include "USBhandler.h"
#include "USBconstant.h"

#if (EP2_ADDR+128) > USER_USB_RAM
#error "This example needs more USB ram. Increase this setting in menu."
#endif

void USB_EP2_IN();
void USB_EP2_OUT();
void USB_EP3_IN();
void USB_EP3_OUT();

// MIDI functions:
void resetMIDIParameters();

// CDC functions:
void resetCDCParameters();
void setLineCodingHandler();
uint16_t getLineCodingHandler();
void setControlLineStateHandler();

__xdata __at (EP0_ADDR) uint8_t  Ep0Buffer[8];     
__xdata __at (EP1_ADDR) uint8_t  Ep1Buffer[8];       // on page 47 of data sheet, the receive buffer need to be min(possible packet size+2,64)
__xdata __at (EP2_ADDR) uint8_t  Ep2Buffer[128];  // EP Buffer for EP2+EP3. CDC+MIDI IN/OUT buffer, must be even address 
// EP2 Ep2Buffer[  0- 31] CDC-IN 
// EP2 Ep2Buffer[ 64- 95] CDC-OUT
// EP3 Ep2Buffer[ 32- 63] MIDI-IN
// EP3 Ep2Buffer[ 96-127] MIDI-OUT

__xdata uint8_t  serDescStr[22] = { 22 ,0x03,
    'c',0x00,'h',0x00,'e',0x00,'e',0x00,'n',0x00,'-',0x00,'2',0x00,'2',0x00,'2',0x00,'2',0x00}; 

uint8_t descSerFlg = 0;
uint8_t *pSerDescr;

uint16_t SetupLen;
uint8_t SetupReq,UsbConfig;

__code uint8_t *pDescr;

volatile uint8_t usbMsgFlags=0;    // uint8_t usbMsgFlags copied from VUSB

inline void NOP_Process(void) {}

__code uint8_t hexStr[]={'0','1','2','3','4','5','6','7','8','9','A','B','C','D','E','F'};

void convInt2Hex(uint8_t *p, uint8_t val){
  uint8_t l = (val % 16);
  uint8_t h = (val / 16);
  *(p+0) = hexStr[l];
  *(p+1) = 0x00;
  *(p+2) = hexStr[h];
  *(p+3) = 0x00;
}

void loadSerialNumber(){
  const __code uint8_t *px = (const __code uint8_t *)ROM_CHIP_ID_HX;
  const __code uint8_t *p = (const __code uint8_t *)ROM_CHIP_ID_LO;
  uint8_t idx = (uint8_t)*(px + 0);
  uint8_t id0 = (uint8_t)*(p + 0);
  uint8_t id1 = (uint8_t)*(p + 1);
  uint8_t id2 = (uint8_t)*(p + 2);
  uint8_t id3 = (uint8_t)*(p + 3);
  convInt2Hex(&(serDescStr[18]),id0);
  convInt2Hex(&(serDescStr[14]),id1);
  convInt2Hex(&(serDescStr[10]),id2);
  convInt2Hex(&(serDescStr[6]),id3);
  convInt2Hex(&(serDescStr[2]),idx);
}

void USB_EP0_SETUP(){
  uint8_t len = USB_RX_LEN;
  if(len == (sizeof(USB_SETUP_REQ))){
    SetupLen = ((uint16_t)UsbSetupBuf->wLengthH<<8) | (UsbSetupBuf->wLengthL);
    len = 0;                                                // Default is success and upload 0 length
    SetupReq = UsbSetupBuf->bRequest;
    if((UsbSetupBuf->bRequestType & USB_REQ_TYP_MASK) != USB_REQ_TYP_STANDARD){//non-standard request
      switch((UsbSetupBuf->bRequestType & USB_REQ_TYP_MASK)){
      case USB_REQ_TYP_VENDOR:    
      {
        switch(SetupReq){
        default:
          len = 0xFF;                                                                        //command not supported
          break;
        }
        break;
      }
      case USB_REQ_TYP_CLASS:
      {
        switch(SetupReq){
        case GET_LINE_CODING:   //0x21  currently configured
          len = getLineCodingHandler();
          break;
        case SET_CONTROL_LINE_STATE:  //0x22  generates RS-232/V.24 style control signals
          setControlLineStateHandler();
          break;
        case SET_LINE_CODING:      //0x20  Configure
          break;
        default:
          len = 0xFF;                                                                        //command not supported
          break;
        }
        break;
      }
      default:
        len = 0xFF;                                                                        //command not supported
        break;
      }
    }else{                                             // standard request
      switch(SetupReq){                                    //request code
      case USB_GET_DESCRIPTOR:
        descSerFlg = 0;
        switch(UsbSetupBuf->wValueH){
        case 1:                                         //device descriptor
          pDescr = DevDesc;                           //Send the device descriptor to the buffer to be sent
          len = DevDescLen;
          break;
        case 2:                                         //configuration descriptor
          pDescr = CfgDesc;                           //Send the configuration descriptor to the buffer to be sent
          len = CfgDescLen;
          break;
        case 3:
          if(UsbSetupBuf->wValueL == 0){
            pDescr = LangDes;
            len = LangDesLen;
          }else if(UsbSetupBuf->wValueL == 1){
            pDescr = Manuf_Des;
            len = Manuf_DesLen;
          }else if(UsbSetupBuf->wValueL == 2){
            pDescr = Prod_Des;
            len = Prod_DesLen;
          }else if(UsbSetupBuf->wValueL == 3){
            pSerDescr = (uint8_t *)serDescStr;
            descSerFlg = 1;
            len = (uint8_t)sizeof(serDescStr);
          }
          break;
        default:
          len = 0xff;                                 //Unsupported command or error
          break;
        }
        if(len != 0xff){
          if(SetupLen > len){
            SetupLen = len;                             //limit the total length
          }
          len = SetupLen >= DEFAULT_ENDP0_SIZE ? DEFAULT_ENDP0_SIZE : SetupLen;   //The length of this transfer
          if(descSerFlg == 1){
            memcpy(Ep0Buffer,serDescStr,len);                   //load upload data
            pSerDescr += len;
          }else{
            memcpy(Ep0Buffer,pDescr,len);                   //load upload data
            pDescr += len;
          }
          SetupLen -= len;
        }
        break;
      case USB_SET_ADDRESS:
        SetupLen = UsbSetupBuf->wValueL;                //Temporary USB device address
        break;
      case USB_GET_CONFIGURATION:
        Ep0Buffer[0] = UsbConfig;
        if(SetupLen >= 1){
          len = 1;
        }
        break;
      case USB_SET_CONFIGURATION:
        UsbConfig = UsbSetupBuf->wValueL;
        break;
      case USB_GET_INTERFACE:
        break;
      case USB_SET_INTERFACE:
        break;
      case USB_CLEAR_FEATURE:                             //Clear Feature
        if((UsbSetupBuf->bRequestType & 0x1F) == USB_REQ_RECIP_DEVICE){    /* clear device */
          if((((uint16_t)UsbSetupBuf->wValueH << 8) | UsbSetupBuf->wValueL) == 0x01){
            if( CfgDesc[ 7 ] & 0x20 ){
                   /* wake */
            }else{
              len = 0xFF;                                     /* operation failed */
            }
          }else{
            len = 0xFF;                                         /* operation failed */
          }
        }else if((UsbSetupBuf->bRequestType & USB_REQ_RECIP_MASK) == USB_REQ_RECIP_ENDP){ // endpoint
          switch(UsbSetupBuf->wIndexL){
          case 0x84:
            UEP4_CTRL = UEP4_CTRL & ~ ( bUEP_T_TOG | MASK_UEP_T_RES ) | UEP_T_RES_NAK;
            break;
          case 0x04:
            UEP4_CTRL = UEP4_CTRL & ~ ( bUEP_R_TOG | MASK_UEP_R_RES ) | UEP_R_RES_ACK;
            break;
          case 0x83:
            UEP3_CTRL = UEP3_CTRL & ~ ( bUEP_T_TOG | MASK_UEP_T_RES ) | UEP_T_RES_NAK;
            break;
          case 0x03:
            UEP3_CTRL = UEP3_CTRL & ~ ( bUEP_R_TOG | MASK_UEP_R_RES ) | UEP_R_RES_ACK;
            break;
          case 0x82:
            UEP2_CTRL = UEP2_CTRL & ~ ( bUEP_T_TOG | MASK_UEP_T_RES ) | UEP_T_RES_NAK;
            break;
          case 0x02:
            UEP2_CTRL = UEP2_CTRL & ~ ( bUEP_R_TOG | MASK_UEP_R_RES ) | UEP_R_RES_ACK;
            break;
          case 0x81:
            UEP1_CTRL = UEP1_CTRL & ~ ( bUEP_T_TOG | MASK_UEP_T_RES ) | UEP_T_RES_NAK;
            break;
          case 0x01:
            UEP1_CTRL = UEP1_CTRL & ~ ( bUEP_R_TOG | MASK_UEP_R_RES ) | UEP_R_RES_ACK;
            break;
          default:
            len = 0xFF;                                     // unsupported endpoint
            break;
          }
        }else{
          len = 0xFF;                                         // not endpoint does not support
        }
        break;
      case USB_SET_FEATURE:                                       /* Set Feature */
        if((UsbSetupBuf->bRequestType & 0x1F) == USB_REQ_RECIP_DEVICE){                /* set up device */
          if((((uint16_t)UsbSetupBuf->wValueH << 8) | UsbSetupBuf->wValueL) == 0x01){
            if(CfgDesc[7] & 0x20){
              while(XBUS_AUX & bUART0_TX){
                ;   //Wait for the send to complete
              }
              SAFE_MOD = 0x55;
              SAFE_MOD = 0xAA;
              WAKE_CTRL = bWAK_BY_USB | bWAK_RXD0_LO | bWAK_RXD1_LO;  //Can be woken up when there is a signal from USB or RXD0/1
              PCON |= PD;                                             //sleep
              SAFE_MOD = 0x55;
              SAFE_MOD = 0xAA;
              WAKE_CTRL = 0x00;
            }else{
              len = 0xFF;                                     /* operation failed */
            }
          }else{
            len = 0xFF;                                         /* operation failed */
          }
        }else if((UsbSetupBuf->bRequestType & 0x1F) == USB_REQ_RECIP_ENDP){            /* set endpoint */
          if((((uint16_t)UsbSetupBuf->wValueH << 8) | UsbSetupBuf->wValueL) == 0x00){
            switch(((uint16_t)UsbSetupBuf->wIndexH << 8 ) | UsbSetupBuf->wIndexL){
            case 0x84:
              UEP4_CTRL = UEP4_CTRL & (~bUEP_T_TOG) | UEP_T_RES_STALL;// Set endpoint4 IN STALL 
              break;
            case 0x04:
              UEP4_CTRL = UEP4_CTRL & (~bUEP_R_TOG) | UEP_R_RES_STALL;// Set endpoint4 OUT Stall 
              break;
            case 0x83:
              UEP3_CTRL = UEP3_CTRL & (~bUEP_T_TOG) | UEP_T_RES_STALL;/* SET ENDPOINT 3 IN STALL */
              break;
            case 0x03:
              UEP3_CTRL = UEP3_CTRL & (~bUEP_R_TOG) | UEP_R_RES_STALL;/* Set endpoint 3 OUT Stall */
              break;
            case 0x82:
              UEP2_CTRL = UEP2_CTRL & (~bUEP_T_TOG) | UEP_T_RES_STALL;/* SET ENDPOINT 2 IN STALL */
              break;
            case 0x02:
              UEP2_CTRL = UEP2_CTRL & (~bUEP_R_TOG) | UEP_R_RES_STALL;/* Set endpoint 2 OUT Stall*/
              break;
            case 0x81:
              UEP1_CTRL = UEP1_CTRL & (~bUEP_T_TOG) | UEP_T_RES_STALL;/* SET ENDPOINT 1 IN STALL */
              break;
            case 0x01:
              UEP1_CTRL = UEP1_CTRL & (~bUEP_R_TOG) | UEP_R_RES_STALL;/* Set endpoint 1 OUT Stall*/
              break;
            default:
              len = 0xFF;                                 /* operation failed */
              break;
            }
          }else{
            len = 0xFF;                                   /* operation failed */
          }
        }else{
          len = 0xFF;                                       /* operation failed */
        }
        break;
      case USB_GET_STATUS:
        Ep0Buffer[0] = 0x00;
        Ep0Buffer[1] = 0x00;
        if(SetupLen >= 2){
          len = 2;
        }else{
          len = SetupLen;
        }
        break;
      default:
        len = 0xff;                                                 //operation failed
        break;
      }
    }
  }else{
    len = 0xff;                                                      //packet length error
  }
  if(len == 0xff){
    SetupReq = 0xFF;
    UEP0_CTRL = bUEP_R_TOG | bUEP_T_TOG | UEP_R_RES_STALL | UEP_T_RES_STALL;//STALL
  }else if(len <= DEFAULT_ENDP0_SIZE){                          //Upload data or return a 0-length packet in the status stage
    UEP0_T_LEN = len;
    UEP0_CTRL = bUEP_R_TOG | bUEP_T_TOG | UEP_R_RES_ACK | UEP_T_RES_ACK;//The default data packet is DATA1, and the response ACK is returned
  }else{
    UEP0_T_LEN = 0;  //Although it has not yet reached the state stage, the upload of 0-length data packets is preset in advance to prevent the host from entering the state stage in advance
    UEP0_CTRL = bUEP_R_TOG | bUEP_T_TOG | UEP_R_RES_ACK | UEP_T_RES_ACK;//The default data packet is DATA1, and the response ACK is returned
  }
}

void USB_EP0_IN(){
  switch(SetupReq){
  case USB_GET_DESCRIPTOR:
  {
    uint8_t len = SetupLen >= DEFAULT_ENDP0_SIZE ? DEFAULT_ENDP0_SIZE : SetupLen;                                 //send length
    if(descSerFlg == 1){
      memcpy( Ep0Buffer, pSerDescr, len);                   //load upload data
      pSerDescr += len;
    }else{
      memcpy( Ep0Buffer, pDescr, len );                                  
      pDescr += len;
    }
    SetupLen -= len;
    UEP0_T_LEN = len;
    UEP0_CTRL ^= bUEP_T_TOG;                    //Switch between DATA0 and DATA1
  }
    break;
  case USB_SET_ADDRESS:
    USB_DEV_AD = USB_DEV_AD & bUDA_GP_BIT | SetupLen;
    UEP0_CTRL = UEP_R_RES_ACK | UEP_T_RES_NAK;
    break;
  default:
    UEP0_T_LEN = 0;                                                      // End of transaction
    UEP0_CTRL = UEP_R_RES_ACK | UEP_T_RES_NAK;
    break;
  }
}

void USB_EP1_IN(){
  UEP1_T_LEN = 0;
  UEP1_CTRL = UEP1_CTRL & ~ MASK_UEP_T_RES | UEP_T_RES_NAK;   //NAK by default
}

void USB_EP0_OUT(){
  if(SetupReq == SET_LINE_CODING){  //Set line coding
    if(U_TOG_OK){
      setLineCodingHandler();
      UEP0_T_LEN = 0;
      UEP0_CTRL |= UEP_R_RES_ACK | UEP_T_RES_ACK;  // send 0-length packet
    }
  }else{
    UEP0_T_LEN = 0;
    UEP0_CTRL |= UEP_R_RES_ACK | UEP_T_RES_NAK;  //Respond Nak
  }
}

#pragma save
#pragma nooverlay
void USBInterrupt(void) {   //inline not really working in multiple files in SDCC
  if(UIF_TRANSFER) {// Dispatch to service functions
    uint8_t callIndex=USB_INT_ST & MASK_UIS_ENDP;
    switch(USB_INT_ST & MASK_UIS_TOKEN){
    case UIS_TOKEN_OUT:
    {//SDCC will take IRAM if array of function pointer is used.
      switch(callIndex){
      case 0: EP0_OUT_Callback(); break;
      case 1: EP1_OUT_Callback(); break;
      case 2: EP2_OUT_Callback(); break;
      case 3: EP3_OUT_Callback(); break;
      case 4: EP4_OUT_Callback(); break;
      default: break;
      }
    }
      break;
    case UIS_TOKEN_SOF:
    {//SDCC will take IRAM if array of function pointer is used.
      switch (callIndex) {
      case 0: EP0_SOF_Callback(); break;
      case 1: EP1_SOF_Callback(); break;
      case 2: EP2_SOF_Callback(); break;
      case 3: EP3_SOF_Callback(); break;
      case 4: EP4_SOF_Callback(); break;
      default: break;
      }
    }
      break;
    case UIS_TOKEN_IN:
    {//SDCC will take IRAM if array of function pointer is used.
      switch (callIndex) {
      case 0: EP0_IN_Callback(); break;
      case 1: EP1_IN_Callback(); break;
      case 2: EP2_IN_Callback(); break;
      case 3: EP3_IN_Callback(); break;
      case 4: EP4_IN_Callback(); break;
      default: break;
      }
    }
      break;
    case UIS_TOKEN_SETUP:
    {//SDCC will take IRAM if array of function pointer is used.
      switch (callIndex) {
      case 0: EP0_SETUP_Callback(); break;
      case 1: EP1_SETUP_Callback(); break;
      case 2: EP2_SETUP_Callback(); break;
      case 3: EP3_SETUP_Callback(); break;
      case 4: EP4_SETUP_Callback(); break;
      default: break;
      }
    }
      break;
    }    
    UIF_TRANSFER = 0;                                                     // Clear interrupt flag
  }
  // Device mode USB bus reset
  if(UIF_BUS_RST){
    UEP0_CTRL = UEP_R_RES_ACK | UEP_T_RES_NAK;
    UEP1_CTRL = bUEP_AUTO_TOG | UEP_T_RES_NAK;                //Endpoint 1 automatically flips the sync flag, and IN transaction returns NAK
    UEP2_CTRL = bUEP_AUTO_TOG | UEP_T_RES_NAK | UEP_R_RES_ACK;        //Endpoint 2 automatically flips the sync flag, IN transaction returns NAK, OUT returns ACK
    UEP3_CTRL = bUEP_AUTO_TOG | UEP_T_RES_NAK | UEP_R_RES_ACK;        //Endpoint 3 automatically flips the sync flag, IN transaction returns NAK, OUT returns ACK
    //UEP4_CTRL = UEP_T_RES_NAK | UEP_R_RES_ACK;  //bUEP_AUTO_TOG only work for endpoint 1,2,3
        
    USB_DEV_AD = 0x00;
    UIF_SUSPEND = 0;
    UIF_TRANSFER = 0;
    UIF_BUS_RST = 0;                                                        // Clear interrupt flag
        
    UsbConfig = 0;        //Clear configuration values
    resetMIDIParameters();
    resetCDCParameters();
  }
    
  // USB bus suspend / wake up
  if(UIF_SUSPEND){
    UIF_SUSPEND = 0;
    if(USB_MIS_ST & bUMS_SUSPEND){                                             //hang up
      while(XBUS_AUX & bUART0_TX){
        ;   //Wait for the send to complete
      }
      SAFE_MOD = 0x55;
      SAFE_MOD = 0xAA;
      WAKE_CTRL = bWAK_BY_USB | bWAK_RXD0_LO | bWAK_RXD1_LO;                    //Can be woken up when there is a signal from USB or RXD0/1
      PCON |= PD;                                                              //to sleep
      SAFE_MOD = 0x55;
      SAFE_MOD = 0xAA;
      WAKE_CTRL = 0x00;
    }
  }else {                                                                           //unexpected interruption, impossible situation
    USB_INT_FG = 0xFF;                                                           //clear interrupt flag
  }
}
#pragma restore

void USBDeviceCfg(){
  USB_CTRL = 0x00;                                                           //Clear USB control register
  USB_CTRL &= ~bUC_HOST_MODE;                                                //This bit is the device selection mode
  USB_CTRL |=  bUC_DEV_PU_EN | bUC_INT_BUSY | bUC_DMA_EN;                    //USB device and internal pull-up enable, automatically return to NAK before interrupt flag is cleared during interrupt
  USB_DEV_AD = 0x00;                                                         //Device address initialization
//     USB_CTRL |= bUC_LOW_SPEED;
//     UDEV_CTRL |= bUD_LOW_SPEED;                                                //Run for 1.5M
  USB_CTRL &= ~bUC_LOW_SPEED;
  UDEV_CTRL &= ~bUD_LOW_SPEED;                                             //Select full speed 12M mode, default mode
#if defined(CH551) || defined(CH552) || defined(CH549)
  UDEV_CTRL = bUD_PD_DIS;                                                     // Disable DP/DM pull-down resistor
#endif
#if defined(CH559)
  UDEV_CTRL = bUD_DP_PD_DIS;                                                     // Disable DP/DM pull-down resistor
#endif
  UDEV_CTRL |= bUD_PORT_EN;                                                  //Enable physical port
}

void USBDeviceIntCfg(){
  USB_INT_EN |= bUIE_SUSPEND;                                               //Enable device hang interrupt
  USB_INT_EN |= bUIE_TRANSFER;                                              //Enable USB transfer completion interrupt
  USB_INT_EN |= bUIE_BUS_RST;                                               //Enable device mode USB bus reset interrupt
  USB_INT_FG |= 0x1F;                                                       //Clear interrupt flag
  IE_USB = 1;                                                               //Enable USB interrupt
  EA = 1;                                                                   //Enable global interrupts
}

void USBDeviceEndPointCfg(){
  UEP1_DMA = (uint16_t) Ep1Buffer;                                                      //Endpoint 1 data transfer address
  UEP2_DMA = (uint16_t) Ep2Buffer;                                                      //Endpoint 2 data transfer address
  UEP3_DMA = (uint16_t) (Ep2Buffer+32);                                                 //Endpoint 3 data transfer address

  UEP2_3_MOD = 0xCC;   // @@@@@ change 0x0C -> 0xCC                                                     //Endpoint2 double buffer
  UEP1_CTRL = bUEP_AUTO_TOG | UEP_T_RES_NAK;                //Endpoint 1 automatically flips the sync flag, and IN transaction returns NAK
  UEP2_CTRL = bUEP_AUTO_TOG | UEP_T_RES_NAK | UEP_R_RES_ACK;        //Endpoint 2 automatically flips the sync flag, IN transaction returns NAK, OUT returns ACK

  UEP3_CTRL = bUEP_AUTO_TOG | UEP_T_RES_NAK | UEP_R_RES_ACK;        //Endpoint 3 automatically flips the sync flag, IN transaction returns NAK, OUT returns ACK
    
  UEP0_DMA = (uint16_t) Ep0Buffer;                                                      //Endpoint 0 data transfer address
  UEP4_1_MOD = 0x40;                                                         //endpoint1 TX enable
  UEP0_CTRL = UEP_R_RES_ACK | UEP_T_RES_NAK;                //Manual flip, OUT transaction returns ACK, IN transaction returns NAK
}
