#include <Arduino.h>
#include <Audio.h>
#include <Wire.h>
#include <SPI.h>
#include <SD.h>
#include <SerialFlash.h>
#include <MIDI.h>
#include <EEPROM.h>
#include "Voice.h"
#include "mixerExt.h" // mixers with other input counts

//#include "GrandPiano_samples.h"
#include "VelocityGrandPiano_samples.h"
#include "MmmmHumSynth_samples.h"
#include "ObieSynth1_samples.h"
//#include "16layerspiano_samples.h"
#include "StereoGrand1_samples.h"

#define InstrumentCount 5

#define VOICE_COUNT 32


#define NOTE_PRESSED_STATE_LED  6
#define NOTE_OVERFLOWN_LED      5

class Synth
{
    public:
    AudioOutputPT8211_2           i2s1;           //xy=1049,465

    const float DIV127 = (1.0 / 127.0);
    const float DIV100 = 0.01;
    const float DIV64 = (1.0/64.0);
    const float DIV360BY127 = (360.0/127.0);
    const float DIV360BY120 = (3.0);

    byte oscAwaveform = WAVEFORM_SINE;
    byte oscBwaveform = WAVEFORM_SINE;
    byte oscCwaveform = WAVEFORM_SINE;

    byte mixVoices_gains = 100;

    byte oscAamp = 100;
    byte oscBamp = 100;
    byte oscCamp = 100;
    byte oscDamp = 100;

    byte oscApulsewidth = 0;
    byte oscBpulsewidth = 0;
    byte oscCpulsewidth = 0;

    byte oscAphase = 0;
    byte oscBphase = 0;
    byte oscCphase = 0;

    byte envDelay = 0;
    byte envAttack = 0;
    byte envHold = 0;
    byte envDecay = 0;
    byte envSustain = 100;
    byte envRelease = 0;

    byte currentWTinstrument = 0;

    AudioMixer32 mixVoices;

    // Static arrays, and those declared directly in a namespace (outside any function), 
    // are always initialized. If no explicit initializer is specified, 
    // all the elements are default-initialized (with zeroes, for fundamental types).
    Voice voices[32] ;
    
    //AudioConnection ac(mixVoices, i2s1); as an example this don't work in classes
    AudioConnection *acVoices[32]; // so audio connections need to be pointers,
    AudioConnection *patchCordOutL; // and initialized in the constructor,
    AudioConnection *patchCordOutR; // but thats much better because the we can use a for-loop

    Synth()
    {
        for (int i= 0 ; i < VOICE_COUNT; i++)
        {
            acVoices[i] = new AudioConnection(voices[i].env, 0, mixVoices, i);
        }
        patchCordOutL = new AudioConnection(mixVoices, 0, i2s1, 0);
        patchCordOutR = new AudioConnection(mixVoices, 0, i2s1, 1);
    }
     
    void noteOn(byte note, byte velocity)
    {
        digitalWrite(NOTE_PRESSED_STATE_LED, HIGH);
        for (int i = 0; i < VOICE_COUNT; i++) 
        {
            if (voices[i].note == note) // first check if the note was played recently
            {
                voices[i].noteOn(note, velocity);
                digitalWrite(NOTE_OVERFLOWN_LED, LOW);
                return; 
            }
        }
        for (int i = 0; i < VOICE_COUNT; i++) // second see if there is any free "spot"
        {
            if (voices[i].isNotPlaying())
            {
                voices[i].noteOn(note, velocity);
                digitalWrite(NOTE_OVERFLOWN_LED, LOW);
                return;
            }
        }
        digitalWrite(NOTE_OVERFLOWN_LED, HIGH);
    }
    void noteOff(byte note, byte velocity)
    {
        digitalWrite(NOTE_PRESSED_STATE_LED, LOW);
        for (int i = 0; i < VOICE_COUNT; i++)
        {
            if (voices[i].note == note)
            {
                voices[i].noteOff(velocity);
                return;
            }
        }
    }
    void activateSustain()
    {
        for (int i = 0; i < VOICE_COUNT; i++)
        {
            voices[i].isSustain = 1;

        }
    }
    void deactivateSustain()
    {
        for (int i = 0; i < VOICE_COUNT; i++)
        {
            voices[i].isSustain = 0;
            if (!voices[i].isNoteOn)
                voices[i].noteOff(0);
        }
    }

    void Begin(void)
    {
        pinMode(NOTE_OVERFLOWN_LED, OUTPUT);
        digitalWrite(NOTE_OVERFLOWN_LED, LOW);

        pinMode(NOTE_PRESSED_STATE_LED, OUTPUT);
        digitalWrite(NOTE_PRESSED_STATE_LED, LOW);

        EEPROM_ReadSettings();

        set_InstrumentByIndex(1); // VelocityGrandPiano
    }
    void set_InstrumentByIndex(byte index)
    {
        currentWTinstrument = index;
        switch(index)
        {
            case 0:
                SetWaveForm_As_Primary();
                break;
            case 1:
                SetWaveTable_As_Primary();
                set_Instrument(VelocityGrandPiano);
                //synth_set_Instrument(_16layerspiano);
                break;
            case 2:
                SetWaveTable_As_Primary();
                set_Instrument(StereoGrand1);
                break;
            case 3:
                SetWaveTable_As_Primary();
                set_Instrument(ObieSynth1);
                break;
            case 4:
                SetWaveTable_As_Primary();
                set_Instrument(MmmmHumSynth);
                break;
            default:
                break;
        }
    }
    void set_Instrument(const AudioSynthWavetable::instrument_data &instrument)
    {
        for (int i = 0; i< VOICE_COUNT; i++)
        {
            voices[i].waveTable.setInstrument(instrument);
            voices[i].waveTable.amplitude(1.0);
        }
    }
    void set_OSC_A_waveform(byte wf)
    {
        if (wf > 8) wf = 8;
        oscAwaveform = wf;
        for (int i = 0; i < VOICE_COUNT; i++)
        {
            voices[i].oscA.begin(wf);
        }
    }
    void set_OSC_B_waveform(byte wf)
    {
        if (wf > 8) wf = 8;
        oscBwaveform = wf;
        for (int i = 0; i < VOICE_COUNT; i++)
        {
            voices[i].oscB.begin(wf);
        }
    }
    void set_OSC_C_waveform(byte wf)
    {
        if (wf > 8) wf = 8;
        oscCwaveform = wf;
        for (int i = 0; i < VOICE_COUNT; i++)
        {
            voices[i].oscC.begin(wf);

            //voices[i].oscD.begin(wf); // experimental
        }
    }
    void set_OSC_A_pulseWidth(byte value)
    {
        if (value > 100) value = 100;
        oscApulsewidth = value;
        for (int i = 0; i < VOICE_COUNT; i++)
        {
            voices[i].oscA.pulseWidth(value*DIV100);
        }
    }
    void set_OSC_B_pulseWidth(byte value)
    {
        if (value > 100) value = 100;
        oscBpulsewidth = value;
        for (int i = 0; i < VOICE_COUNT; i++)
        {
            voices[i].oscB.pulseWidth(value*DIV100);
        }
    }
    void set_OSC_C_pulseWidth(byte value)
    {
        if (value > 100) value = 100;
        oscCpulsewidth = value;
        for (int i = 0; i < VOICE_COUNT; i++)
        {
            voices[i].oscC.pulseWidth(value*DIV100);
        }
    }
    void set_OSC_A_phase(byte value)
    {
        if (value > 120) value = 120;
        oscAphase = value;
        for (int i = 0; i < VOICE_COUNT; i++)
        {
            voices[i].oscA.phase(value*DIV360BY120);
        }
    }
    void set_OSC_B_phase(byte value)
    {
        if (value > 120) value = 120;
        oscBphase = value;
        for (int i = 0; i < VOICE_COUNT; i++)
        {
            voices[i].oscB.phase(value*DIV360BY120);
        }
    }
    void set_OSC_C_phase(byte value)
    {
        if (value > 120) value = 120;
        oscCphase = value;
        for (int i = 0; i < VOICE_COUNT; i++)
        {
            voices[i].oscC.phase(value*DIV360BY120);
        }
    }
    void set_OSC_A_amplitude(byte value)
    {
        if (value > 100) value = 100;
        oscAamp = value;
        for (int i = 0; i < VOICE_COUNT; i++)
        {
            voices[i].mix.gain(0,value*DIV100);
        }
    }
    void set_OSC_B_amplitude(byte value)
    {
        if (value > 100) value = 100;
        oscBamp = value;
        for (int i = 0; i < VOICE_COUNT; i++)
        {
            voices[i].mix.gain(1, value*DIV100);
        }
    }
    void set_OSC_C_amplitude(byte value)
    {
        if (value > 100) value = 100;
        oscCamp = value;
        for (int i = 0; i < VOICE_COUNT; i++)
        {
            voices[i].mix.gain(2, value*DIV100);
        }
    }
    void set_OSC_D_amplitude(byte value)
    {
        if (value > 100) value = 100;
        oscDamp = value;
        for (int i = 0; i < VOICE_COUNT; i++)
        {
            voices[i].mix.gain(3, value*DIV100);
        }
    }
    void set_mixVoices_gains(byte value)
    {
        if (value > 100) value = 100;
        mixVoices_gains = value;
        for (int i = 0; i < VOICE_COUNT; i++)
            mixVoices.gain(i, value*DIV100);
    }

    void set_envelope_delay(byte value)
    {
        if (value > 127) value = 127;
        envDelay = value;
        for (int i = 0; i < VOICE_COUNT; i++)
        {
            voices[i].env.delay((float)value*(float)10);
        }
    }
    void set_envelope_attack(byte value)
    {
        if (value > 127) value = 127;
        envAttack = value;
        for (int i = 0; i < VOICE_COUNT; i++)
        {
            voices[i].env.attack((float)value*(float)10);
        }
    }
    void set_envelope_hold(byte value)
    {
        if (value > 127) value = 127;
        envHold = value;
        for (int i = 0; i < VOICE_COUNT; i++)
        {
            voices[i].env.hold((float)value*(float)10);
        }
    }
    void set_envelope_decay(byte value)
    {
        if (value > 127) value = 127;
        envDecay = value;
        for (int i = 0; i < VOICE_COUNT; i++)
        {
            voices[i].env.decay((float)value*(float)10);
        }
    }
    void set_envelope_sustain(byte value)
    {
        if (value > 100) value = 100;
        envSustain = value;
        for (int i = 0; i < VOICE_COUNT; i++)
        {
            voices[i].env.sustain(value*DIV100);
        }
    }
    void set_envelope_release(byte value)
    {
        if (value > 127) value = 127;
        envRelease = value;
        for (int i = 0; i < VOICE_COUNT; i++)
        {
            voices[i].env.release((float)value*(float)10);
        }
    }
    void set_OSC_A_freqMult(byte value)
    {
        if (value > 127) value = 64;
        oscApitchMult = value;
    }
    void set_OSC_B_freqMult(byte value)
    {
        if (value > 127) value = 64;
        oscBpitchMult = value;
    }
    void set_OSC_C_freqMult(byte value)
    {
        if (value > 127) value = 64;
        oscCpitchMult = value;
    }

    void SetWaveTable_As_Primary()
    {
        set_OSC_A_amplitude(0);
        set_OSC_B_amplitude(0);
        set_OSC_C_amplitude(0);
        set_OSC_D_amplitude(100);
        set_mixVoices_gains(100);
    }

    void SetWaveForm_As_Primary()
    {
        set_OSC_A_amplitude(100);
        set_OSC_B_amplitude(100);
        set_OSC_C_amplitude(100);
        set_OSC_D_amplitude(0);
        set_mixVoices_gains(3);
    }

    void sendAllSettings()
    {
        usbMIDI.sendControlChange(0, currentWTinstrument, 0x00);
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
        usbMIDI.sendControlChange(32, oscDamp, 0x00);
        usbMIDI.sendControlChange(33, mixVoices_gains, 0x00);
        usbMIDI.sendControlChange(100, envDelay, 0x00);
        usbMIDI.sendControlChange(101, envAttack, 0x00);
        usbMIDI.sendControlChange(102, envHold, 0x00);
        usbMIDI.sendControlChange(103, envDecay, 0x00);
        usbMIDI.sendControlChange(104, envSustain, 0x00);
        usbMIDI.sendControlChange(105, envRelease, 0x00);
            
        usbMIDI.sendControlChange(108, oscApitchMult, 0x00);
        usbMIDI.sendControlChange(109, oscBpitchMult, 0x00);
        usbMIDI.sendControlChange(110, oscCpitchMult, 0x00);
    }

    void EEPROM_SaveSettings()
    {
        EEPROM.write(20, oscAwaveform);
        EEPROM.write(21, oscBwaveform);
        EEPROM.write(22, oscCwaveform);

        EEPROM.write(23, oscApulsewidth);
        EEPROM.write(24, oscBpulsewidth);
        EEPROM.write(25, oscCpulsewidth);

        EEPROM.write(26, oscAphase);
        EEPROM.write(27, oscBphase);
        EEPROM.write(28, oscCphase);

        EEPROM.write(29, oscAamp);
        EEPROM.write(30, oscBamp);
        EEPROM.write(31, oscCamp);
        EEPROM.write(32, oscDamp);

        EEPROM.write(33, mixVoices_gains);

        EEPROM.write(100, envDelay);
        EEPROM.write(101, envAttack);
        EEPROM.write(102, envHold);
        EEPROM.write(103, envDecay);
        EEPROM.write(104, envSustain);
        EEPROM.write(105, envRelease);
            
        EEPROM.write(108, oscApitchMult);
        EEPROM.write(109, oscBpitchMult);
        EEPROM.write(110, oscCpitchMult);
    }
    void EEPROM_ReadSettings()
    {
        set_OSC_A_waveform(EEPROM.read(20));
        set_OSC_B_waveform(EEPROM.read(21));
        set_OSC_C_waveform(EEPROM.read(22));

        set_OSC_A_pulseWidth(EEPROM.read(23));
        set_OSC_B_pulseWidth(EEPROM.read(24));
        set_OSC_C_pulseWidth(EEPROM.read(25));

        set_OSC_A_phase(EEPROM.read(26));
        set_OSC_B_phase(EEPROM.read(27));
        set_OSC_C_phase(EEPROM.read(28));
        
        set_OSC_A_amplitude(EEPROM.read(29));
        set_OSC_B_amplitude(EEPROM.read(30));
        set_OSC_C_amplitude(EEPROM.read(31));

        set_mixVoices_gains(EEPROM.read(33));
            
        set_envelope_delay(EEPROM.read(100));
        set_envelope_attack(EEPROM.read(101));
        set_envelope_hold(EEPROM.read(102));
        set_envelope_decay(EEPROM.read(103));
        set_envelope_sustain(EEPROM.read(104));
        set_envelope_release(EEPROM.read(105));
            
        set_OSC_A_freqMult(EEPROM.read(108));
        set_OSC_A_freqMult(EEPROM.read(109));
        set_OSC_A_freqMult(EEPROM.read(110));
    }
};
