#include <Arduino.h>
#include <Audio.h>
#include <Wire.h>
#include <SPI.h>
#include <SD.h>
#include <SerialFlash.h>
#include <MIDI.h>
#include <EEPROM.h>

#define VOICE_COUNT 32

#define NOTE_OVERFLOWN_LED 6

AudioOutputPT8211_2           i2s1;           //xy=1049,465

const float noteFreqs[128] = {8.176, 8.662, 9.177, 9.723, 10.301, 10.913, 11.562, 12.25, 12.978, 13.75, 14.568, 15.434, 16.352, 17.324, 18.354, 19.445, 20.602, 21.827, 23.125, 24.5, 25.957, 27.5, 29.135, 30.868, 32.703, 34.648, 36.708, 38.891, 41.203, 43.654, 46.249, 48.999, 51.913, 55, 58.27, 61.735, 65.406, 69.296, 73.416, 77.782, 82.407, 87.307, 92.499, 97.999, 103.826, 110, 116.541, 123.471, 130.813, 138.591, 146.832, 155.563, 164.814, 174.614, 184.997, 195.998, 207.652, 220, 233.082, 246.942, 261.626, 277.183, 293.665, 311.127, 329.628, 349.228, 369.994, 391.995, 415.305, 440, 466.164, 493.883, 523.251, 554.365, 587.33, 622.254, 659.255, 698.456, 739.989, 783.991, 830.609, 880, 932.328, 987.767, 1046.502, 1108.731, 1174.659, 1244.508, 1318.51, 1396.913, 1479.978, 1567.982, 1661.219, 1760, 1864.655, 1975.533, 2093.005, 2217.461, 2349.318, 2489.016, 2637.02, 2793.826, 2959.955, 3135.963, 3322.438, 3520, 3729.31, 3951.066, 4186.009, 4434.922, 4698.636, 4978.032, 5274.041, 5587.652, 5919.911, 6271.927, 6644.875, 7040, 7458.62, 7902.133, 8372.018, 8869.844, 9397.273, 9956.063, 10548.08, 11175.3, 11839.82, 12543.85};
const float DIV127 = (1.0 / 127.0);
const float DIV100 = 0.01;
const float DIV64 = (1.0/64.0);
const float DIV360BY127 = (360.0/127.0);
const float DIV360BY120 = (3.0);

byte oscAwaveform = WAVEFORM_SINE;
byte oscBwaveform = WAVEFORM_SINE;
byte oscCwaveform = WAVEFORM_SINE;

byte mix1_gains = 100;
byte mix2_gains = 100;
byte mix3_gains = 100;

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

byte oscAfreqMult = 64; // set at middle
byte oscBfreqMult = 64; // set at middle
byte oscCfreqMult = 64; // set at middle

void synth_set_InstrumentByIndex(byte index);
void synth_set_Instrument(const AudioSynthWavetable::instrument_data &instrument);

void synth_set_OSC_A_waveform(byte value);
void synth_set_OSC_B_waveform(byte value);
void synth_set_OSC_C_waveform(byte value);

void synth_set_OSC_A_amplitude(byte value);
void synth_set_OSC_B_amplitude(byte value);
void synth_set_OSC_C_amplitude(byte value);
void synth_set_OSC_D_amplitude(byte value);

void synth_set_mix1_gains(byte value);
void synth_set_mix2_gains(byte value);
void synth_set_mix3_gains(byte value);


void synth_set_OSC_A_pulseWidth(byte value);
void synth_set_OSC_B_pulseWidth(byte value);
void synth_set_OSC_C_pulseWidth(byte value);

void synth_set_OSC_A_phase(byte value0);
void synth_set_OSC_B_phase(byte value);
void synth_set_OSC_C_phase(byte value);
    
void synth_set_envelope_delay(byte value);
void synth_set_envelope_attack(byte value);
void synth_set_envelope_hold(byte value);
void synth_set_envelope_decay(byte value);
void synth_set_envelope_sustain(byte value);
void synth_set_envelope_release(byte value);

void synth_set_OSC_A_freqMult(byte value);
void synth_set_OSC_B_freqMult(byte value);
void synth_set_OSC_C_freqMult(byte value);

void synth_SetWaveTable_As_Primary();
void synth_SetWaveForm_As_Primary();

void synth_sendAllSettings();

void synth_EEPROM_SaveSettings();
void synth_EEPROM_ReadSettings();

class Voice
{
  public:
    AudioSynthWaveform oscA;
    AudioSynthWaveform oscB;
    AudioSynthWaveform oscC;
    AudioSynthWavetable waveTable;

    //AudioSynthWaveform oscDA; // experimental
    //AudioSynthWaveform oscDB; // experimental
    //AudioSynthWaveformModulated oscD; // experimental

    AudioMixer4 mix;
    AudioEffectEnvelope env;

    AudioConnection *pcOscA;
    AudioConnection *pcOscB;
    AudioConnection *pcOscC;
    AudioConnection *pcWaveTable;

    //AudioConnection *pcOscDA; // experimental
    //AudioConnection *pcOscDB; // experimental
    //AudioConnection *pcOscD; // experimental

    AudioConnection *patchCordMix;
    
    Voice()
    {
      pcOscA = new AudioConnection(oscA, 0, mix, 0);
      pcOscB = new AudioConnection(oscB, 0, mix, 1);
      pcOscC = new AudioConnection(oscC, 0, mix, 2);
      pcWaveTable = new AudioConnection(waveTable, 0, mix, 3);
      
      

      //pcOscDA = new AudioConnection(oscA, 0, oscD, 0); // experimental
      //pcOscDB = new AudioConnection(oscB, 0, oscD, 1); // experimental
      //pcOscD = new AudioConnection(oscD, 0, mix, 3); // experimental

      patchCordMix = new AudioConnection(mix, 0, env, 0);
    }
    void noteOn(byte Note, byte velocity)
    {
      note = Note;
      isNoteOn = 1;
      newFreqA = GetNewFreq(oscAfreqMult);
      newFreqB = GetNewFreq(oscBfreqMult);
      newFreqC = GetNewFreq(oscCfreqMult);

      //newFreqD = noteFreqs[note]; // experimental
      
      //velocity -= 63; // change of velocity curve
      // newAmp = (float)velocity*DIV64; // change of velocity curve

      newAmp = (float)velocity*DIV127;
      
      oscA.frequency(newFreqA);
      oscB.frequency(newFreqB);
      oscC.frequency(newFreqC);

      //oscD.frequency(newFreqD); // experimental
      
      oscA.amplitude(newAmp);
      oscB.amplitude(newAmp);
      oscC.amplitude(newAmp);

      //oscD.amplitude(newAmp); // experimental

      waveTable.playNote(note, velocity);
      
      env.noteOn();
    }
    void noteOff(byte velocity)
    {
        isNoteOn = 0;
        if (!isSustain)
        {
            env.noteOff();
            waveTable.stop(1000);
        }
        
      
      //note = 0;
    }
    bool isNotPlaying()
    {
        if (!env.isActive())
            return true;
        else if(!waveTable.isPlaying())
            return true;
        else
            return false;
    }
    byte note = 0;
    byte isNoteOn = 0;
    byte isSustain = 0;
    float newAmp = 0.0;
    float newFreqA = 0.0;
    float newFreqB = 0.0;
    float newFreqC = 0.0;
    //float newFreqD = 0.0; // experimental

    float GetNewFreq(byte freqMult)
    {
        if (freqMult <= 64)
            return noteFreqs[note] * (1/(((float)64 - freqMult) + 1));
        else
            return noteFreqs[note] * ((freqMult - (float)64) + 1);
    }
};

Voice voices[] = {Voice(), Voice(), Voice(), Voice(), Voice(), Voice(), Voice(), Voice(), Voice(), Voice(), Voice(), Voice(), Voice(), Voice(), Voice(), Voice(),
                  Voice(), Voice(), Voice(), Voice(), Voice(), Voice(), Voice(), Voice(), Voice(), Voice(), Voice(), Voice(), Voice(), Voice(), Voice(), Voice()};

void noteOn(byte note, byte velocity)
{
    digitalWrite(ledPin, HIGH);
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
    digitalWrite(ledPin, LOW);
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

AudioMixer4 mix1a;
AudioMixer4 mix1b;
AudioMixer4 mix1c;
AudioMixer4 mix1d;
AudioMixer4 mix1e;
AudioMixer4 mix1f;
AudioMixer4 mix1g;
AudioMixer4 mix1h;

AudioMixer4 mix2a;
AudioMixer4 mix2b;

AudioMixer4 mix3;
//AudioEffectFreeverb reverb;


AudioConnection patchCordVoice01(voices[0].env, 0, mix1a , 0);
AudioConnection patchCordVoice02(voices[1].env, 0, mix1a , 1);
AudioConnection patchCordVoice03(voices[2].env, 0, mix1a , 2);
AudioConnection patchCordVoice04(voices[3].env, 0, mix1a , 3);
AudioConnection patchCordVoice05(voices[4].env, 0, mix1b , 0);
AudioConnection patchCordVoice06(voices[5].env, 0, mix1b , 1);
AudioConnection patchCordVoice07(voices[6].env, 0, mix1b , 2);
AudioConnection patchCordVoice08(voices[7].env, 0, mix1b , 3);
AudioConnection patchCordVoice09(voices[8].env, 0, mix1c , 0);
AudioConnection patchCordVoice10(voices[9].env, 0, mix1c , 1);
AudioConnection patchCordVoice11(voices[10].env, 0, mix1c , 2);
AudioConnection patchCordVoice12(voices[11].env, 0, mix1c , 3);
AudioConnection patchCordVoice13(voices[12].env, 0, mix1d , 0);
AudioConnection patchCordVoice14(voices[13].env, 0, mix1d , 1);
AudioConnection patchCordVoice15(voices[14].env, 0, mix1d , 2);
AudioConnection patchCordVoice16(voices[15].env, 0, mix1d , 3);

AudioConnection patchCordVoice17(voices[16].env, 0, mix1e , 0);
AudioConnection patchCordVoice18(voices[17].env, 0, mix1e , 1);
AudioConnection patchCordVoice19(voices[18].env, 0, mix1e , 2);
AudioConnection patchCordVoice20(voices[19].env, 0, mix1e , 3);
AudioConnection patchCordVoice21(voices[20].env, 0, mix1f , 0);
AudioConnection patchCordVoice22(voices[21].env, 0, mix1f , 1);
AudioConnection patchCordVoice23(voices[22].env, 0, mix1f , 2);
AudioConnection patchCordVoice24(voices[23].env, 0, mix1f , 3);
AudioConnection patchCordVoice25(voices[24].env, 0, mix1g , 0);
AudioConnection patchCordVoice26(voices[25].env, 0, mix1g , 1);
AudioConnection patchCordVoice27(voices[26].env, 0, mix1g , 2);
AudioConnection patchCordVoice28(voices[27].env, 0, mix1g , 3);
AudioConnection patchCordVoice29(voices[28].env, 0, mix1h , 0);
AudioConnection patchCordVoice30(voices[29].env, 0, mix1h , 1);
AudioConnection patchCordVoice31(voices[30].env, 0, mix1h , 2);
AudioConnection patchCordVoice32(voices[31].env, 0, mix1h , 3);
/*
AudioConnection patchCordVoice1(voices[0].mix, 0, mix1a , 0);
AudioConnection patchCordVoice2(voices[1].mix, 0, mix1a , 1);
AudioConnection patchCordVoice3(voices[2].mix, 0, mix1a , 2);
AudioConnection patchCordVoice4(voices[3].mix, 0, mix1a , 3);
AudioConnection patchCordVoice5(voices[4].mix, 0, mix1b , 0);
AudioConnection patchCordVoice6(voices[5].mix, 0, mix1b , 1);
AudioConnection patchCordVoice7(voices[6].mix, 0, mix1b , 2);
AudioConnection patchCordVoice8(voices[7].mix, 0, mix1b , 3);
*/
AudioConnection patchCordMix1a(mix1a, 0, mix2a, 0);
AudioConnection patchCordMix1b(mix1b, 0, mix2a, 1);
AudioConnection patchCordMix1c(mix1c, 0, mix2a, 2);
AudioConnection patchCordMix1d(mix1d, 0, mix2a, 3);
AudioConnection patchCordMix1e(mix1e, 0, mix2b, 0);
AudioConnection patchCordMix1f(mix1f, 0, mix2b, 1);
AudioConnection patchCordMix1g(mix1g, 0, mix2b, 2);
AudioConnection patchCordMix1h(mix1h, 0, mix2b, 3);

AudioConnection patchCordMix2a(mix2a, 0, mix3, 0);
AudioConnection patchCordMix2b(mix2b, 0, mix3, 1);

//AudioConnection pc1(mix2, 0, reverb, 0);

AudioConnection patchCordOutL(mix3, 0, i2s1, 0);
AudioConnection patchCordOutR(mix3, 0, i2s1, 1);

void synth_Init(void)
{
    pinMode(NOTE_OVERFLOWN_LED, OUTPUT);
    digitalWrite(NOTE_OVERFLOWN_LED, LOW);
    synth_set_Instrument(VelocityGrandPiano);
    
    synth_EEPROM_ReadSettings();
}
void synth_set_InstrumentByIndex(byte index)
{
    switch(index)
    {
        case 0:
            synth_set_Instrument(VelocityGrandPiano);
            break;
        case 1:
            //synth_set_Instrument(GrandPiano);
            break;
        case 2:
            synth_set_Instrument(MmmmHumSynth);
            break;
        case 3:
            synth_set_Instrument(ObieSynth1);
            break;
        case 4:
            
            break;
        case 5:
            
            break;
        default:
            break;
    }
}
void synth_set_Instrument(const AudioSynthWavetable::instrument_data &instrument)
{
    for (int i = 0; i< VOICE_COUNT; i++)
    {
        voices[i].waveTable.setInstrument(instrument);
        voices[i].waveTable.amplitude(1.0);
    }
}
void synth_set_OSC_A_waveform(byte wf)
{
    if (wf > 8) wf = 8;
    oscAwaveform = wf;
    for (int i = 0; i < VOICE_COUNT; i++)
    {
        voices[i].oscA.begin(wf);
    }
}
void synth_set_OSC_B_waveform(byte wf)
{
    if (wf > 8) wf = 8;
    oscBwaveform = wf;
    for (int i = 0; i < VOICE_COUNT; i++)
    {
        voices[i].oscB.begin(wf);
    }
}
void synth_set_OSC_C_waveform(byte wf)
{
    if (wf > 8) wf = 8;
    oscCwaveform = wf;
    for (int i = 0; i < VOICE_COUNT; i++)
    {
        voices[i].oscC.begin(wf);

        //voices[i].oscD.begin(wf); // experimental
    }
}
void synth_set_OSC_A_pulseWidth(byte value)
{
    if (value > 100) value = 100;
    oscApulsewidth = value;
    for (int i = 0; i < VOICE_COUNT; i++)
    {
        voices[i].oscA.pulseWidth(value*DIV100);
    }
}
void synth_set_OSC_B_pulseWidth(byte value)
{
    if (value > 100) value = 100;
    oscBpulsewidth = value;
    for (int i = 0; i < VOICE_COUNT; i++)
    {
        voices[i].oscB.pulseWidth(value*DIV100);
    }
}
void synth_set_OSC_C_pulseWidth(byte value)
{
    if (value > 100) value = 100;
    oscCpulsewidth = value;
    for (int i = 0; i < VOICE_COUNT; i++)
    {
        voices[i].oscC.pulseWidth(value*DIV100);
    }
}
void synth_set_OSC_A_phase(byte value)
{
    if (value > 120) value = 120;
    oscAphase = value;
    for (int i = 0; i < VOICE_COUNT; i++)
    {
        voices[i].oscA.phase(value*DIV360BY120);
    }
}
void synth_set_OSC_B_phase(byte value)
{
    if (value > 120) value = 120;
    oscBphase = value;
    for (int i = 0; i < VOICE_COUNT; i++)
    {
        voices[i].oscB.phase(value*DIV360BY120);
    }
}
void synth_set_OSC_C_phase(byte value)
{
    if (value > 120) value = 120;
    oscCphase = value;
    for (int i = 0; i < VOICE_COUNT; i++)
    {
        voices[i].oscC.phase(value*DIV360BY120);
    }
}
void synth_set_OSC_A_amplitude(byte value)
{
    if (value > 100) value = 100;
    oscAamp = value;
    for (int i = 0; i < VOICE_COUNT; i++)
    {
        voices[i].mix.gain(0,value*DIV100);
    }
}
void synth_set_OSC_B_amplitude(byte value)
{
    if (value > 100) value = 100;
    oscBamp = value;
    for (int i = 0; i < VOICE_COUNT; i++)
    {
        voices[i].mix.gain(1, value*DIV100);
    }
}
void synth_set_OSC_C_amplitude(byte value)
{
    if (value > 100) value = 100;
    oscCamp = value;
    for (int i = 0; i < VOICE_COUNT; i++)
    {
        voices[i].mix.gain(2, value*DIV100);
    }
}
void synth_set_OSC_D_amplitude(byte value)
{
    if (value > 100) value = 100;
    oscDamp = value;
    for (int i = 0; i < VOICE_COUNT; i++)
    {
        voices[i].mix.gain(3, value*DIV100);
    }
}
void synth_set_mix1_gains(byte value)
{
    if (value > 100) value = 100;
    mix1_gains = value;
    mix1a.gain(0, value*DIV100);
    mix1a.gain(1, value*DIV100);
    mix1a.gain(2, value*DIV100);
    mix1a.gain(3, value*DIV100);

    mix1b.gain(0, value*DIV100);
    mix1b.gain(1, value*DIV100);
    mix1b.gain(2, value*DIV100);
    mix1b.gain(3, value*DIV100);

    mix1c.gain(0, value*DIV100);
    mix1c.gain(1, value*DIV100);
    mix1c.gain(2, value*DIV100);
    mix1c.gain(3, value*DIV100);

    mix1d.gain(0, value*DIV100);
    mix1d.gain(1, value*DIV100);
    mix1d.gain(2, value*DIV100);
    mix1d.gain(3, value*DIV100);

    mix1e.gain(0, value*DIV100);
    mix1e.gain(1, value*DIV100);
    mix1e.gain(2, value*DIV100);
    mix1e.gain(3, value*DIV100);

    mix1f.gain(0, value*DIV100);
    mix1f.gain(1, value*DIV100);
    mix1f.gain(2, value*DIV100);
    mix1f.gain(3, value*DIV100);

    mix1g.gain(0, value*DIV100);
    mix1g.gain(1, value*DIV100);
    mix1g.gain(2, value*DIV100);
    mix1g.gain(3, value*DIV100);

    mix1h.gain(0, value*DIV100);
    mix1h.gain(1, value*DIV100);
    mix1h.gain(2, value*DIV100);
    mix1h.gain(3, value*DIV100);
}

void synth_set_mix2_gains(byte value)
{
    if (value > 100) value = 100;
    mix2_gains = value;
    mix2a.gain(0, value*DIV100);
    mix2a.gain(1, value*DIV100);
    mix2a.gain(2, value*DIV100);
    mix2a.gain(3, value*DIV100);

    mix2b.gain(0, value*DIV100);
    mix2b.gain(1, value*DIV100);
    mix2b.gain(2, value*DIV100);
    mix2b.gain(3, value*DIV100);
}
void synth_set_mix3_gains(byte value)
{
    if (value > 100) value = 100;
    mix3_gains = value;
    mix3.gain(0, value*DIV100);
    mix3.gain(1, value*DIV100);
}

void synth_set_envelope_delay(byte value)
{
    if (value > 127) value = 127;
    envDelay = value;
    for (int i = 0; i < VOICE_COUNT; i++)
    {
        voices[i].env.delay((float)value*(float)10);
    }
}
void synth_set_envelope_attack(byte value)
{
    if (value > 127) value = 127;
    envAttack = value;
    for (int i = 0; i < VOICE_COUNT; i++)
    {
        voices[i].env.attack((float)value*(float)10);
    }
}
void synth_set_envelope_hold(byte value)
{
    if (value > 127) value = 127;
    envHold = value;
    for (int i = 0; i < VOICE_COUNT; i++)
    {
        voices[i].env.hold((float)value*(float)10);
    }
}
void synth_set_envelope_decay(byte value)
{
    if (value > 127) value = 127;
    envDecay = value;
    for (int i = 0; i < VOICE_COUNT; i++)
    {
        voices[i].env.decay((float)value*(float)10);
    }
}
void synth_set_envelope_sustain(byte value)
{
    if (value > 100) value = 100;
    envSustain = value;
    for (int i = 0; i < VOICE_COUNT; i++)
    {
        voices[i].env.sustain(value*DIV100);
    }
}
void synth_set_envelope_release(byte value)
{
    if (value > 127) value = 127;
    envRelease = value;
    for (int i = 0; i < VOICE_COUNT; i++)
    {
        voices[i].env.release((float)value*(float)10);
    }
}
void synth_set_OSC_A_freqMult(byte value)
{
    if (value > 127) value = 64;
    oscAfreqMult = value;
}
void synth_set_OSC_B_freqMult(byte value)
{
    if (value > 127) value = 64;
    oscBfreqMult = value;
}
void synth_set_OSC_C_freqMult(byte value)
{
    if (value > 127) value = 64;
    oscCfreqMult = value;
}

void synth_SetWaveTable_As_Primary()
{
    synth_set_OSC_A_amplitude(0);
    synth_set_OSC_B_amplitude(0);
    synth_set_OSC_C_amplitude(0);
    synth_set_OSC_D_amplitude(100);
    synth_set_mix1_gains(100);
    synth_set_mix2_gains(100);
    synth_set_mix3_gains(100);
}

void synth_SetWaveForm_As_Primary()
{
    synth_set_OSC_A_amplitude(100);
    synth_set_OSC_B_amplitude(100);
    synth_set_OSC_C_amplitude(100);
    synth_set_OSC_D_amplitude(0);
    synth_set_mix1_gains(25);
    synth_set_mix2_gains(25);
    synth_set_mix3_gains(50);
}

void synth_sendAllSettings()
{
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
    usbMIDI.sendControlChange(33, mix1_gains, 0x00);
    usbMIDI.sendControlChange(34, mix2_gains, 0x00);
    usbMIDI.sendControlChange(35, mix3_gains, 0x00);
    usbMIDI.sendControlChange(100, envDelay, 0x00);
    usbMIDI.sendControlChange(101, envAttack, 0x00);
    usbMIDI.sendControlChange(102, envHold, 0x00);
    usbMIDI.sendControlChange(103, envDecay, 0x00);
    usbMIDI.sendControlChange(104, envSustain, 0x00);
    usbMIDI.sendControlChange(105, envRelease, 0x00);
          
    usbMIDI.sendControlChange(108, oscAfreqMult, 0x00);
    usbMIDI.sendControlChange(109, oscBfreqMult, 0x00);
    usbMIDI.sendControlChange(110, oscCfreqMult, 0x00);
}

void synth_EEPROM_SaveSettings()
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

    EEPROM.write(33, mix1_gains);
    EEPROM.write(34, mix2_gains);
    EEPROM.write(35, mix3_gains);
    

    EEPROM.write(100, envDelay);
    EEPROM.write(101, envAttack);
    EEPROM.write(102, envHold);
    EEPROM.write(103, envDecay);
    EEPROM.write(104, envSustain);
    EEPROM.write(105, envRelease);
          
    EEPROM.write(108, oscAfreqMult);
    EEPROM.write(109, oscBfreqMult);
    EEPROM.write(110, oscCfreqMult);
}
void synth_EEPROM_ReadSettings()
{
    synth_set_OSC_A_waveform(EEPROM.read(20));
    synth_set_OSC_B_waveform(EEPROM.read(21));
    synth_set_OSC_C_waveform(EEPROM.read(22));

    synth_set_OSC_A_pulseWidth(EEPROM.read(23));
    synth_set_OSC_B_pulseWidth(EEPROM.read(24));
    synth_set_OSC_C_pulseWidth(EEPROM.read(25));

    synth_set_OSC_A_phase(EEPROM.read(26));
    synth_set_OSC_B_phase(EEPROM.read(27));
    synth_set_OSC_C_phase(EEPROM.read(28));
    
    synth_set_OSC_A_amplitude(EEPROM.read(29));
    synth_set_OSC_B_amplitude(EEPROM.read(30));
    synth_set_OSC_C_amplitude(EEPROM.read(31));

    synth_set_mix1_gains(EEPROM.read(33));
    synth_set_mix2_gains(EEPROM.read(34));
    synth_set_mix3_gains(EEPROM.read(35));
        
    synth_set_envelope_delay(EEPROM.read(100));
    synth_set_envelope_attack(EEPROM.read(101));
    synth_set_envelope_hold(EEPROM.read(102));
    synth_set_envelope_decay(EEPROM.read(103));
    synth_set_envelope_sustain(EEPROM.read(104));
    synth_set_envelope_release(EEPROM.read(105));
          
    synth_set_OSC_A_freqMult(EEPROM.read(108));
    synth_set_OSC_A_freqMult(EEPROM.read(109));
    synth_set_OSC_A_freqMult(EEPROM.read(110));
}