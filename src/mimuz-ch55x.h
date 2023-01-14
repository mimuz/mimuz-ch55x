#ifndef __MIMUZ_CH55X_H__
#define __MIMUZ_CH55X_H__

#include <stdint.h>
#include "include/ch5xx.h"
#include "include/ch5xx_usb.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef void (*hdlMidiMessage)( __xdata uint8_t, __xdata uint8_t, __xdata uint8_t ) __reentrant;
typedef void (*hdlNoteOff)( __xdata uint8_t, __xdata uint8_t ) __reentrant;

void USBInit(void);
void sendNoteOn(uint8_t ch, uint8_t note, uint8_t vel);
void sendNoteOff(uint8_t ch, uint8_t note);
void sendCtlChange(uint8_t ch, uint8_t num, uint8_t value);
void setHdlNoteOn(hdlMidiMessage);
void setHdlNoteOff(hdlNoteOff);
void setHdlCtlChange(hdlMidiMessage);
void processMidiMessage();

#ifdef __cplusplus
} // extern "C"
#endif

#endif

