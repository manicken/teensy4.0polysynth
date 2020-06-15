// Teensy-Synth Part 9
// 5-PIN MIDI INPUT
// By Notes and Volts
// www.notesandvolts.com
#include <Arduino.h>
#include <Audio.h>
#include <Wire.h>
#include <SPI.h>
#include <SD.h>
#include <SerialFlash.h>
#include <MIDI.h>


const int ledPin = 13;
int ledState = LOW;             // ledState used to set the LED
unsigned long previousMillis = 0;        // will store last time LED was updated
unsigned long currentMillis = 0;
unsigned long currentInterval = 0;
unsigned long ledBlinkOnInterval = 100;
unsigned long ledBlinkOffInterval = 2000;

#define btnInEnablePin 9
#define btnSustainPin 23
#define btnSostenutoPin 22
#define btnSoftPedalPin 21
uint8_t btnSustain = 0;
uint8_t btnSostenuto = 0;
uint8_t btnSoftPedal = 0;
uint8_t btnSustainWasPressed = 0;
uint8_t btnSostenutoWasPressed = 0;
uint8_t btnSoftPedalWasPressed = 0;

#define btnNextInstrumentPin 20
uint8_t btnNextInstrument = 0;
uint8_t btnNextInstrumentWasPressed = 0;

void uartMidi_NoteOn(byte channel, byte note, byte velocity);
void uartMidi_NoteOff(byte channel, byte note, byte velocity);
void uartMidi_ControlChange(byte channel, byte control, byte value);
void uartMidi_PitchBend(byte channel, int value);

void usbMidi_NoteOn(byte channel, byte note, byte velocity);
void usbMidi_NoteOff(byte channel, byte note, byte velocity);
void usbMidi_ControlChange(byte channel, byte control, byte value);
void usbMidi_PitchBend(byte channel, int value);

void blinkLedTask(void);
void btnInputProcessTask(void);

#include "Synth.h"

Synth synth;
  
#define KEYBOARD_NOTE_SHIFT_CORRECTION 21//-12

MIDI_CREATE_INSTANCE(HardwareSerial, Serial1, MIDI);

void setup()
{
  AudioMemory(96);
  
  MIDI.begin();
  MIDI.setHandleNoteOn(uartMidi_NoteOn);
  MIDI.setHandleNoteOff(uartMidi_NoteOff);
  MIDI.setHandleControlChange(uartMidi_ControlChange);
  MIDI.setHandlePitchBend(uartMidi_PitchBend);

  usbMIDI.setHandleNoteOn(usbMidi_NoteOn);
  usbMIDI.setHandleNoteOff(usbMidi_NoteOff);
  usbMIDI.setHandleControlChange(usbMidi_ControlChange);
  usbMIDI.setHandlePitchChange(usbMidi_PitchBend);

  synth.Begin();

  pinMode(btnSustainPin, INPUT);
  pinMode(btnSostenutoPin, INPUT);
  pinMode(btnSoftPedalPin, INPUT);
  pinMode(btnNextInstrumentPin, INPUT);

  pinMode(btnInEnablePin, OUTPUT);
  digitalWrite(btnInEnablePin, LOW);

  pinMode(ledPin, OUTPUT);
  digitalWrite(ledPin, LOW);

  btnSustainWasPressed = 0;
  btnSoftPedalWasPressed = 0;
  btnSostenutoWasPressed = 0;
  btnNextInstrumentWasPressed = 0;
}

void loop()
{
    usbMIDI.read();
    MIDI.read();

    btnInputProcessTask();
    
    blinkLedTask();
}

void uartMidi_NoteOn(byte channel, byte note, byte velocity) {
    note += KEYBOARD_NOTE_SHIFT_CORRECTION;
    velocity = 127 - velocity;
    synth.noteOn(note, velocity);
    usbMIDI.sendNoteOn(note, velocity, channel, 0);
}

void uartMidi_NoteOff(byte channel, byte note, byte velocity) {
    note += KEYBOARD_NOTE_SHIFT_CORRECTION;
    velocity = 127 - velocity;
    synth.noteOff(note, velocity);
    usbMIDI.sendNoteOff(note, velocity, channel, 0);
}

void uartMidi_ControlChange(byte channel, byte control, byte value) {
    usbMIDI.sendControlChange(control, value, channel, 0x00);
}

void uartMidi_PitchBend(byte channel, int value) {
    usbMIDI.sendPitchBend(value, channel, 0x00);
}

void usbMidi_NoteOn(byte channel, byte note, byte velocity) {
    synth.noteOn(note, velocity);
}

void usbMidi_NoteOff(byte channel, byte note, byte velocity) {
    synth.noteOff(note, velocity);    
}

void usbMidi_PitchBend(byte channel, int value) {
  
}

void usbMidi_ControlChange(byte channel, byte control, byte value) {
    switch (control) { // cases 20-31,102-119 is undefined in midi spec
        case 64:
          if (value == 0)
            synth.deactivateSustain();
          else if (value == 127)
            synth.activateSustain();
          break;
        case 0:
          synth.set_InstrumentByIndex(value);
          break;
        case 20: // OSC A waveform select
          synth.set_OSC_A_waveform(value);
          break;
        case 21: // OSC B waveform select
          synth.set_OSC_B_waveform(value);
          break;
        case 22: // OSC C waveform select
          synth.set_OSC_C_waveform(value);
          break;

        case 23:
          synth.set_OSC_A_pulseWidth(value);
          break;
        case 24:
          synth.set_OSC_B_pulseWidth(value);
          break;
        case 25:
          synth.set_OSC_C_pulseWidth(value);
          break;

        case 26:
          synth.set_OSC_A_phase(value);
          break;
        case 27:
          synth.set_OSC_B_phase(value);
          break;
        case 28:
          synth.set_OSC_C_phase(value);
          break;

        case 29:
          synth.set_OSC_A_amplitude(value);
          break;
        case 30:
          synth.set_OSC_B_amplitude(value);
          break;
        case 31:
          synth.set_OSC_C_amplitude(value);
          break;
        case 32: //("LSB for Control 0 (Bank Select)" @ midi spec.)
          synth.set_OSC_D_amplitude(value);
          break;

        case 33: 
          synth.set_mixVoices_gains(value);
          break;
        
        case 100:
          synth.set_envelope_delay(value);
          break;
        case 101:
          synth.set_envelope_attack(value);
          break;
        case 102:
          synth.set_envelope_hold(value);
          break;
        case 103:
          synth.set_envelope_decay(value);
          break;
        case 104:
          synth.set_envelope_sustain(value);
          break;
        case 105:
          synth.set_envelope_release(value);
          break;
                 
        case 108:
          synth.set_OSC_A_freqMult(value);
          break;
        case 109:
          synth.set_OSC_B_freqMult(value);
          break;
        case 110:
          synth.set_OSC_C_freqMult(value);
          break;

        case 115: // set wavetable as primary (Piano mode)
          synth.SetWaveTable_As_Primary();
          break;
        case 116:
          synth.SetWaveForm_As_Primary();
          break;
          
        case 117: // EEPROM read settings
          synth.EEPROM_ReadSettings();
          break;
        case 118: // EEPROM save settings
          synth.EEPROM_SaveSettings();
          break;

        case 119: // get all values
          synth.sendAllSettings();
        break;
    }
}

void btnInputProcessTask(void)
{
  btnSustain = digitalRead(btnSustainPin);
  btnSostenuto = digitalRead(btnSostenutoPin);
  btnSoftPedal = digitalRead(btnSoftPedalPin);
  btnNextInstrument = digitalRead(btnNextInstrumentPin);

    // Sustain pedal
    if ((btnSustain == LOW) && (btnSustainWasPressed == 0))
    {
        btnSustainWasPressed = 1;
        usbMIDI.sendControlChange(0x40, 0x7F, 0x00);
        synth.activateSustain();
    }
    else if ((btnSustain == HIGH) && (btnSustainWasPressed == 1))
    {
        btnSustainWasPressed = 0;
        usbMIDI.sendControlChange(0x40, 0x00, 0x00);
        synth.deactivateSustain();
    }
    // Sostenuto Pedal
    if ((btnSostenuto == LOW) && (btnSostenutoWasPressed == 0))
    {
        btnSostenutoWasPressed = 1;
        usbMIDI.sendControlChange(0x42, 0x7F, 0x00);
    }
    else if ((btnSostenuto == HIGH) && (btnSostenutoWasPressed == 1))
    {
        btnSostenutoWasPressed = 0;
        usbMIDI.sendControlChange(0x42, 0x00, 0x00);
    }
    // Soft Pedal
    if ((btnSoftPedal == LOW) && (btnSoftPedalWasPressed == 0))
    {
        btnSoftPedalWasPressed = 1;
        usbMIDI.sendControlChange(0x43, 0x7F, 0x00);
    }
    else if ((btnSoftPedal == HIGH) && (btnSoftPedalWasPressed == 1))
    {
        btnSoftPedalWasPressed = 0;
        usbMIDI.sendControlChange(0x43, 0x00, 0x00);
    }
    // Next Instrument button
    if ((btnNextInstrument == LOW) && (btnNextInstrumentWasPressed == 0))
    {
        btnNextInstrumentWasPressed = 1;
        if (synth.currentWTinstrument == (InstrumentCount - 1)) synth.currentWTinstrument = 0;
        else synth.currentWTinstrument++;
        synth.set_InstrumentByIndex(synth.currentWTinstrument);
        usbMIDI.sendControlChange(0, synth.currentWTinstrument, 0x00);
    }
    else if ((btnNextInstrument == HIGH) && (btnNextInstrumentWasPressed == 1))
    {
        btnNextInstrumentWasPressed = 0;
    }
}

void blinkLedTask(void)
{
    currentMillis = millis();
    currentInterval = currentMillis - previousMillis;
    
    if (ledState == LOW)
    {
        if (currentInterval > ledBlinkOffInterval)
        {
            previousMillis = currentMillis;
            ledState = HIGH;
            digitalWrite(ledPin, HIGH);
        }
    }
    else
    {
        if (currentInterval > ledBlinkOnInterval)
        {
            previousMillis = currentMillis;
            ledState = LOW;
            digitalWrite(ledPin, LOW);
        }
    }
}