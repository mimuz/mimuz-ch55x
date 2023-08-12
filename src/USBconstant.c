#include "USBconstant.h"


// Device desc
__code uint8_t DevDesc[] = {    
    0x12,      // size
    0x01,      // Decice Desc 
    0x10,0x01, // USB Ver. 1.1
    0xEF,0x02,0x01,  // USB IAD (See. https://www.usb.org/sites/default/files/iadclasscode_r10.pdf)
    DEFAULT_ENDP0_SIZE, /* Define it in interface level */
    0x09,0x12,0x03,0xDF, /* VID PID bString (see https://pid.codes/1209/DF03/)*/
    0x12,0x00,  // BCD Product (0.12)
    0x01,       // iManufacturer (String Desc 1)
    0x02,       // iProduct (String Desc 2)
    0x03,       // iSerialNumber (String Desc 3)
    0x01        // bNumConfigurations = 1
};

__code uint16_t DevDescLen = sizeof(DevDesc);

// Config desc
__code uint8_t CfgDesc[] ={
    // configuration descriptor (4 interfaces)
    0x09,0x02,
    sizeof(CfgDesc) & 0xff,sizeof(CfgDesc) >> 8,    // wTotalLength
    0x04,       // bNumInterfaces = 4
    0x01,       // bConfigurationValue = 1
    0x00,       // iConfiguration (no index)
    0x80,       // bmAttributes 
    0x32,       // MaxPower = 100mA     

  // CDC >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>

    // IAD
    0x08,       // bLength
    0x0B,       // bDescriptorType = 0x0B (IAD)
    0x00,       // bFirstInterface = Interface 0
    0x02,       // bInterfaceCount = 2
    0x02,       // bFunctionClass = 2 (CDC)
    0x02,       // bFunctionSubClass = 2
    0x01,       // bFunctionProtocol = 1
    0x00,       // iInterface = 0 (no index)

    // Interface 0 (CDC) descriptor
    0x09,       // bLength
    0x04,       // bDescriptorType = 4 (Interface Descriptor) 
    0x00,       // bInterfaceNumber (0)
    0x00,       // bAlternateSetting (0)
    0x01,       // bNumEndpoints = 1
    0x02,       // bInterfaceClass = 2 (CDC)
    0x02,       // bInterfaceSubClass = 2 (ACM)
    0x01,       // bInterfaceProtocol = 1 (AT Commands defined by ITU-T V.250 etc)
    0x00,       // iInterface = 0 (no index)

    // Header Functional Descriptor
    0x05,       // bFunctionLength
    0x24,       // bDescriptorType = 0x24 (Interface)
    0x00,       // bDescriptorSubType = 0 (Header Functional Descriptor)
    0x10,0x01,  // bcdCDC (CDC Version 1.10)

    // Call Management Functional Descriptor
    0x05,       // bFunctionLength
    0x24,       // bDescriptorType = 0x24 (Interface)
    0x01,       // bDescriptorSubType = 1 (Call Management Functional Descriptor)
    0x00,       // bmCapabilities
    0x00,       // bDataInterface 

    // ACM Functional Descriptor
    0x04,       // bFunctionLength
    0x24,       // bDescriptorType = 0x24 (Interface)
    0x02,       // bDescriptorSubType = 2 (Abstract Control Management Functional Descriptor)
    0x02,       // bmCapabilities = D1 on = supports the request combination of Set_Line_Coding, Set_Control_Line_State, Get_Line_Coding, and the notification Serial_State

    // Union Functional Descriptor, Communication class interface 0, Data Class Interface 1
    0x05,       // bFunctionLength
    0x24,       // bDescriptorType = 0x24 (Interface)
    0x06,       // bDescriptorSubType = 0x06 (Union Functional Descriptor)
    0x00,       // bControlInterface = 0
    0x01,       // bSubordinateInterface[0] = 1

    // EndPoint descriptor (CDC Upload, Interrupt) IN 1
    0x07,       // bLength
    0x05,       // bDescriptorType = 5 (Endpoint Descriptor)
    0x81,       // bEndpointAddress = 0x81 (Direction=IN EndpointID=1)
    0x03,       // bmAttributes = 0x03 (TransferType=Interrupt)
    0x08,0x00,  // wMaxPacketSize = 8
    0x40,       // bInterval = 0x40 = (64 ms)

    // Interface 1 (Data Interface) descriptor
    0x09,       // bLength
    0x04,       // bDescriptorType = 4 (Interface Descriptor)
    0x01,       // bInterfaceNumber = 1 
    0x00,       // bAlternateSetting
    0x02,       // bNumEndpoints = 2
    0x0a,       // bInterfaceClass = 0x0A (CDC-Data)
    0x00,       // bInterfaceSubClass
    0x00,       // bInterfaceProtocol
    0x00,       // iInterface = 0 (no index)

    // endpoint descriptor OUT 2
    0x07,       // bLength
    0x05,       // bDescriptorType = 5 (Endpoint Descriptor)
    0x02,       // bEndpointAddress = 0x02 (Direction=OUT EndpointID=2)
    0x02,       // bmAttributes = 2 (TransferType=Bulk)
    0x20,0x00,  // wMaxPacketSize = 32 
    0x00,       // bInterval = ignored                

    // endpoint descriptor IN 2
    0x07,       // bLength
    0x05,       // bDescriptorType = 5 (Endpoint Descriptor)
    0x82,       // bEndpointAddress (Direction=IN EndpointID=2)
    0x02,       // bmAttributes = 2 (TransferType=Bulk)
    0x20,0x00,  // wMaxPacketSize = 32 
    0x00,       // bInterval = ignored  

  // MIDI >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>

    // Audio interface descriptor (interface 2)
    0x09,       // bLength 
    0x04,       // bDescriptorType = 4 (Interface Descriptor)
    0x02,       // bInterfaceNumber (2)
    0x00,       // bAlternateSetting (0)
    0x00,       // bNumEndpoints (0)
    0x01,       // bInterfaceClass = 1 (Audio)
    0x01,       // bInterfaceSubClass = 1 (Audio Control) 
    0x00,       // bInterfaceProtocol (0)
    0x00,       // iInterface = 0 (no desc)

    // Audio Control Interface Header Descriptor 
    0x09,       // bLength
    0x24,       // bDescriptorType = 0x24 (Class Specific Interface Descriptor)
    0x01,       // bDescriptorSubtype = 0x01 (Audio Control Interface Header)
    0x00,0x01,  // bcdADC = 0x0100
    0x09,0x00,  // wTotalLength = 0x0009 (9 bytes)
    0x01,       // bInCollection = 1
    0x03,       // baInterfaceNr[1] = 3 ( 1->3 fixed 2023/08/12 )

    // MIDI interface descriptor (interface 3)
    0x09,       // bLength
    0x04,       // bDescriptorType = 4 (Interface Descriptor)
    0x03,       // bInterfaceNumber (3)
    0x00,       // bAlternateSetting (0)
    0x02,       // bNumEndpoints = 2
    0x01,       // bInterfaceClass = 1 (Audio)
    0x03,       // bInterfaceSubClass = 3 (MIDI Streaming)
    0x00,       // bInterfaceProtocol (0)
    0x00,       // iInterface = 0 (no desc)

    // MIDI Adapter Class specific MS Interface Descriptor
    0x07,       // bLength
    0x24,       // bDescriptorType = 0x24 (Class Specific Interface Descriptor)
    0x01,       // bDescriptorSubtype = 0x01 (MS Header)
    0x00,0x01,  // bcdADC = 0x0100
    0x25,0x00,  // wTotalLength = 0x0025

    // MIDI IN Jack Descriptor (Embedded)
    0x06,       // bLength
    0x24,       // bDescriptorType = 0x24 (Class Specific Interface Descriptor)
    0x02,       // bDescriptorSubtype = 0x02 (MIDI IN JACK)
    0x01,       // bJackType = 0x01 (Embedded)
    0x01,       // bJackID = 1
    0x00,       // iJack = 0 (no desc)

    // MIDI IN Jack Descriptor (External)
    0x06,       // bLength
    0x24,       // bDescriptorType = 0x24 (Class Specific Interface Descriptor)
    0x02,       // bDescriptorSubtype = 0x02 (MIDI IN JACK)
    0x02,       // bJackType = 0x02 (External)
    0x02,       // bJackID = 2
    0x00,       // iJack = 0 (no desc)

    // MIDI OUT Jack Descriptor (Embedded)
    0x09,       // bLength
    0x24,       // bDescriptorType = 0x24 (Class Specific Interface Descriptor)
    0x03,       // bDescriptorSubtype = 0x03 (MIDI OUT JACK)
    0x01,       // bJackType = 0x01 (Embedded)
    0x03,       // bJackID = 3
    0x01,       // bNrInputPins = 1
    0x02,       // baSourceID ID=2
    0x01,       // baSourcePin = 1
    0x00,       // iJack = 0 (no desc)

    // MIDI OUT Jack Descriptor (External)
    0x09,       // bLength
    0x24,       // bDescriptorType = 0x24 (Class Specific Interface Descriptor)
    0x03,       // bDescriptorSubtype = 0x03 (MIDI OUT JACK)
    0x02,       // bJackType = 0x02 (External)
    0x04,       // bJackID = 4
    0x01,       // bNrInputPins = 1
    0x01,       // baSourceID ID=1
    0x01,       // baSourcePin = 1
    0x00,       // iJack = 0 (no desc)

    // endpoint descriptor OUT 3
    0x07,       // bLength
    0x05,       // bDescriptorType = 5 (Endpoint Descriptor)
    0x03,       // bEndpointAddress = 0x03 (Direction=OUT EndpointID=3)
    0x02,       // bmAttributes  = 2 (TransferType=Bulk)
    0x20,0x00,  // wMaxPacketSize = 32
    0x00,       // bInterval = 0

    // MS Bulk Data Endpoint Descriptor (EMB MIDI JACK = 1, AssocJACKID=1, OUT)
    0x05,       // bLength
    0x25,       // bDescriptorType = 0x25 (Audio Endpoint Descriptor)
    0x01,       // bDescriptorSubtype = 1 (MS General)
    0x01,       // bNumEmbMIDIJack = 1 (1 embedded MIDI jack)
    0x01,       // baAssocJackID = 1 (ID=1)

    // endpoint descriptor IN 3
    0x07,       // bLength
    0x05,       // bDescriptorType = 5 (Endpoint Descriptor)
    0x83,       // bEndpointAddress = 0x83 (Direction=IN EndpointID=3)
    0x02,       // bmAttributes = 0x02 (TransferType=Bulk)
    0x20,0x00,  // wMaxPacketSize = 32
    0x00,       // bInterval = 0

    // MS Bulk Data Endpoint Descriptor (EMB MIDI JACK = 1, AssocJACKID=3, IN)
    0x05,       // bLength
    0x25,       // bDescriptorType = 0x25 (Audio Endpoint Descriptor)
    0x01,       // bDescriptorSubtype = 1 (MS General)
    0x01,       // bNumEmbMIDIJack = 1 (1 embedded MIDI jack)
    0x03        // baAssocJackID = 3 (ID=3)
};

__code uint16_t CfgDescLen = sizeof(CfgDesc);

// String Descriptors
__code uint8_t LangDes[]={0x04,0x03,0x09,0x04};           // Language Descriptor
__code uint16_t LangDesLen = sizeof(LangDes);

/*
__code uint8_t SerDes[]={                                 // Serial String Descriptor
    0x1C,0x03,'c',0x00,'h',0x00,'e',0x00,'e',0x00,'n',0x00,'-',0x00,'0',0x00,'8',0x00,'0',0x00,'-',0x00,'0',0x00,'0',0x00,'0',0x00};
__code uint16_t SerDesLen = sizeof(SerDes);
*/

__code uint8_t Prod_Des[]={                                // Produce String Descriptor
    0x1A,0x03,'m',0x00,'i',0x00,':',0x00,'m',0x00,'u',0x00,'z',0x00,'-',0x00,'C',0x00,'H',0x00,'5',0x00,'5',0x00,'x',0x00};
__code uint16_t Prod_DesLen = sizeof(Prod_Des);

__code uint8_t Manuf_Des[]={
    0x1E,0x03,
    'T',0x00,'r',0x00,'i',0x00,'p',0x00,'A',0x00,'r',0x00,'t',0x00,'s',0x00,' ',0x00,'M',0x00,'u',0x00,'s',0x00,'i',0x00,'c',0x00};
__code uint16_t Manuf_DesLen = sizeof(Manuf_Des);
