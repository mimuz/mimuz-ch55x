#ifndef __MIMUZ_CH55X_H__
#define __MIMUZ_CH55X_H__

#include <stdint.h>
#include "include/ch5xx.h"
#include "include/ch5xx_usb.h"

#ifdef __cplusplus
extern "C" {
#endif

enum MidiControlChangeNumber
 {
     // High resolution Continuous Controllers MSB (+32 for LSB) ----------------
     BankSelect                  = 0,
     ModulationWheel             = 1,
     BreathController            = 2,
     // CC3 undefined
     FootController              = 4,
     PortamentoTime              = 5,
     DataEntryMSB                = 6,
     ChannelVolume               = 7,
     Balance                     = 8,
     // CC9 undefined
     Pan                         = 10,
     ExpressionController        = 11,
     EffectControl1              = 12,
     EffectControl2              = 13,
     // CC14 undefined
     // CC15 undefined
     GeneralPurposeController1   = 16,
     GeneralPurposeController2   = 17,
     GeneralPurposeController3   = 18,
     GeneralPurposeController4   = 19,
  
     DataEntryLSB                = 38,
  
     // Switches ----------------------------------------------------------------
     Sustain                     = 64,
     Portamento                  = 65,
     Sostenuto                   = 66,
     SoftPedal                   = 67,
     Legato                      = 68,
     Hold                        = 69,
  
     // Low resolution continuous controllers -----------------------------------
     SoundController1            = 70,   
     SoundController2            = 71,   
     SoundController3            = 72,   
     SoundController4            = 73,   
     SoundController5            = 74,   
     SoundController6            = 75,   
     SoundController7            = 76,   
     SoundController8            = 77,   
     SoundController9            = 78,   
     SoundController10           = 79,   
     GeneralPurposeController5   = 80,
     GeneralPurposeController6   = 81,
     GeneralPurposeController7   = 82,
     GeneralPurposeController8   = 83,
     PortamentoControl           = 84,
     // CC85 to CC90 undefined
     Effects1                    = 91,   
     Effects2                    = 92,   
     Effects3                    = 93,   
     Effects4                    = 94,   
     Effects5                    = 95,   
     DataIncrement               = 96,
     DataDecrement               = 97,
     NRPNLSB                     = 98,   
     NRPNMSB                     = 99,   
     RPNLSB                      = 100,  
     RPNMSB                      = 101,  
  
     // Channel Mode messages ---------------------------------------------------
     AllSoundOff                 = 120,
     ResetAllControllers         = 121,
     LocalControl                = 122,
     AllNotesOff                 = 123,
     OmniModeOff                 = 124,
     OmniModeOn                  = 125,
     MonoModeOn                  = 126,
     PolyModeOn                  = 127
 };

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

