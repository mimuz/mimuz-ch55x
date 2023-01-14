/*
  USB MIDI callback example on CH552

  created 2022 by @tadfmac for use with CH55xduino

  This example code is in the public domain.
*/

#ifndef USER_USB_RAM
#error "This example needs to be compiled with a USER USB setting"
#endif

#include "mimuz-ch55x.h"

#define LED_PIN 30

void onNoteOn(__xdata uint8_t ch, __xdata uint8_t note, __xdata uint8_t vel) {
  digitalWrite(LED_PIN,HIGH);
  sendNoteOn(ch,note,vel);  // loopback
  USBSerial_println("onNoteOn");
}

void onNoteOff(__xdata uint8_t ch, __xdata uint8_t note) {
  digitalWrite(LED_PIN,LOW);
  sendNoteOff(ch,note); // loopback
  USBSerial_println("onNoteOff");
}

void setup() {
  pinMode(LED_PIN,OUTPUT);
  digitalWrite(LED_PIN,LOW);

  setHdlNoteOn(onNoteOn);
  setHdlNoteOff(onNoteOff);

  USBInit();
}

void loop() {
  processMidiMessage();
  USBSerial_flush();
}
