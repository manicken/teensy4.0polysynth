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

const int btnInEnablePin = 23;
const int btnSustainPin = 22;
byte btnSustainWasPressed = 0;

void uartMidi_NoteOn(byte channel, byte note, byte velocity);
void uartMidi_NoteOff(byte channel, byte note, byte velocity);
void uartMidi_ControlChange(byte channel, byte control, byte value);
void uartMidi_PitchBend(byte channel, int value);

void usbMidi_NoteOn(byte channel, byte note, byte velocity);
void usbMidi_NoteOff(byte channel, byte note, byte velocity);
void usbMidi_ControlChange(byte channel, byte control, byte value);
void usbMidi_PitchBend(byte channel, int value);

void blinkLedTask(void);
void pedalProcessTask(void);

#include "Synth.h"
  
#define KEYBOARD_NOTE_SHIFT_CORRECTION 21//-12

MIDI_CREATE_INSTANCE(HardwareSerial, Serial1, MIDI);

void setup()
{
  AudioMemory(264);
  
  MIDI.begin();
  MIDI.setHandleNoteOn(uartMidi_NoteOn);
  MIDI.setHandleNoteOff(uartMidi_NoteOff);
  MIDI.setHandleControlChange(uartMidi_ControlChange);
  MIDI.setHandlePitchBend(uartMidi_PitchBend);

  usbMIDI.setHandleNoteOn(usbMidi_NoteOn);
  usbMIDI.setHandleNoteOff(usbMidi_NoteOff);
  usbMIDI.setHandleControlChange(usbMidi_ControlChange);
  usbMIDI.setHandlePitchChange(usbMidi_PitchBend);

  synth_Init();

  pinMode(btnSustainPin, INPUT);
  pinMode(btnInEnablePin, OUTPUT);
  digitalWrite(btnInEnablePin, LOW);

  pinMode(ledPin, OUTPUT);
  digitalWrite(ledPin, LOW);

  btnSustainWasPressed = 0;

  
}

void loop()
{
    usbMIDI.read();
    MIDI.read();

    pedalProcessTask();
    
    blinkLedTask();
}

void uartMidi_NoteOn(byte channel, byte note, byte velocity) {
    note += KEYBOARD_NOTE_SHIFT_CORRECTION;
    velocity = 127 - velocity;
    noteOn(note, velocity);
    usbMIDI.sendNoteOn(note, velocity, channel, 0);
}

void uartMidi_NoteOff(byte channel, byte note, byte velocity) {
    note += KEYBOARD_NOTE_SHIFT_CORRECTION;
    velocity = 127 - velocity;
    noteOff(note, velocity);
    usbMIDI.sendNoteOff(note, velocity, channel, 0);
}

void uartMidi_ControlChange(byte channel, byte control, byte value) {
    usbMIDI.sendControlChange(control, value, channel, 0x00);
}

void uartMidi_PitchBend(byte channel, int value) {
    usbMIDI.sendPitchBend(value, channel, 0x00);
}

void usbMidi_NoteOn(byte channel, byte note, byte velocity) {
    noteOn(note, velocity);
}

void usbMidi_NoteOff(byte channel, byte note, byte velocity) {
    noteOff(note, velocity);    
}

void usbMidi_PitchBend(byte channel, int value) {
  
}

void usbMidi_ControlChange(byte channel, byte control, byte value) {
    switch (control) { // cases 20-31,102-119 is undefined in midi spec
        case 20: // OSC A waveform select
          synth_set_OSC_A_waveform(value);
          break;

        case 21: // OSC B waveform select
          synth_set_OSC_B_waveform(value);
          break;

        case 22: // OSC C waveform select
          synth_set_OSC_C_waveform(value);
          break;

        case 23:
          synth_set_OSC_A_pulseWidth(value);
          break;

        case 24:
          synth_set_OSC_B_pulseWidth(value);
          break;

        case 25:
          synth_set_OSC_C_pulseWidth(value);
          break;

        case 26:
          synth_set_OSC_A_phase(value);
          break;

        case 27:
          synth_set_OSC_B_phase(value);
          break;

        case 28:
          synth_set_OSC_C_phase(value);
          break;

        case 29:
          synth_set_OSC_A_amplitude(value);
          break;

        case 30:
          synth_set_OSC_B_amplitude(value);
          break;

        case 31:
          synth_set_OSC_C_amplitude(value);
          break;

        case 102:
          synth_set_envelope_attack(value);
          break;
        
        case 103:
          synth_set_envelope_decay(value);
          break;

        case 104:
          synth_set_envelope_sustain(value);
          break;

        case 105:
          synth_set_envelope_release(value);
          break;

        case 106:
          synth_set_envelope_delay(value);
          break;

        case 107:
          synth_set_envelope_hold(value);
          break;

        case 108:
          synth_set_OSC_A_freqMult(value);
          break;

        case 109:
          synth_set_OSC_B_freqMult(value);
          break;

        case 110:
          synth_set_OSC_C_freqMult(value);
          break;

        case 119: // get all values
          usbMIDI.sendControlChange(20, oscAwaveform, 0x00);
          usbMIDI.sendControlChange(21, oscBwaveform, 0x00);
          usbMIDI.sendControlChange(22, oscCwaveform, 0x00);
          usbMIDI.sendControlChange(23, oscApulsewidth, 0x00);
          usbMIDI.sendControlChange(24, oscBpulsewidth, 0x00);
          usbMIDI.sendControlChange(25, oscCpulsewidth, 0x00);
          usbMIDI.sendControlChange(26, oscAphase, 0x00);
          usbMIDI.sendControlChange(27, oscBphase, 0x00);
          usbMIDI.sendControlChange(28, oscCphase, 0x00);
          usbMIDI.sendControlChange(29, oscAamp, 0x00);
          usbMIDI.sendControlChange(30, oscBamp, 0x00);
          usbMIDI.sendControlChange(31, oscCamp, 0x00);
          usbMIDI.sendControlChange(102, envAttack, 0x00);
          usbMIDI.sendControlChange(103, envDecay, 0x00);
          usbMIDI.sendControlChange(104, envSustain, 0x00);
          usbMIDI.sendControlChange(105, envRelease, 0x00);
          usbMIDI.sendControlChange(106, envDelay, 0x00);
          usbMIDI.sendControlChange(107, envHold, 0x00);
          usbMIDI.sendControlChange(108, oscAfreqMult, 0x00);
          usbMIDI.sendControlChange(109, oscBfreqMult, 0x00);
          usbMIDI.sendControlChange(110, oscCfreqMult, 0x00);
        break;
    }
}

void pedalProcessTask(void)
{
  if ((digitalRead(btnSustainPin) == LOW) && (btnSustainWasPressed == 0))
    {
        btnSustainWasPressed = 1;
        usbMIDI.sendControlChange(0x40, 0x7F, 0x00);
        //usbMIDI.sendSysEx(5, "HELLO");
    }
    else if ((digitalRead(btnSustainPin) == HIGH) && (btnSustainWasPressed == 1))
    {
        btnSustainWasPressed = 0;
        usbMIDI.sendControlChange(0x40, 0x00, 0x00);
        //usbMIDI.sendSysEx(5, "HELLO");
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