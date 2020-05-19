#include <Arduino.h>
#include <Audio.h>
#include <Wire.h>
#include <SPI.h>
#include <SD.h>
#include <SerialFlash.h>
#include <MIDI.h>

#define VOICE_COUNT 16

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

byte mix1a_gain = 100;
byte mix1b_gain = 100;
byte mix1c_gain = 100;
byte mix1d_gain = 100;
byte mix2_gain0 = 100;
byte mix2_gain1 = 100;
byte mix2_gain2 = 100;
byte mix2_gain3 = 100;

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

void synth_set_OSC_A_waveform(byte value);
void synth_set_OSC_B_waveform(byte value);
void synth_set_OSC_C_waveform(byte value);

void synth_set_OSC_A_amplitude(byte value);
void synth_set_OSC_B_amplitude(byte value);
void synth_set_OSC_C_amplitude(byte value);
void synth_set_OSC_D_amplitude(byte value);

void synth_set_mix1a_gain(byte value);
void synth_set_mix1b_gain(byte value);
void synth_set_mix1c_gain(byte value);
void synth_set_mix1d_gain(byte value);

void synth_set_mix2_gain0(byte value);
void synth_set_mix2_gain1(byte value);
void synth_set_mix2_gain2(byte value);
void synth_set_mix2_gain3(byte value);

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

Voice voices[] = {Voice(), Voice(), Voice(), Voice(), Voice(), Voice(), Voice(), Voice(), Voice(), Voice(), Voice(), Voice(), Voice(), Voice(), Voice(), Voice()};

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
AudioMixer4 mix2;
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
AudioConnection patchCordMix1a(mix1a, 0, mix2, 0);
AudioConnection patchCordMix1b(mix1b, 0, mix2, 1);
AudioConnection patchCordMix1c(mix1c, 0, mix2, 2);
AudioConnection patchCordMix1d(mix1d, 0, mix2, 3);
//AudioConnection pc1(mix2, 0, reverb, 0);

AudioConnection patchCordOutL(mix2, 0, i2s1, 0);
AudioConnection patchCordOutR(mix2, 0, i2s1, 1);

void synth_Init(void)
{
    pinMode(NOTE_OVERFLOWN_LED, OUTPUT);
    digitalWrite(NOTE_OVERFLOWN_LED, LOW);

    for (int i = 0; i< VOICE_COUNT; i++)
    {
        voices[i].waveTable.setInstrument(GrandPiano);
        voices[i].waveTable.amplitude(1.0);
    }

    mix1a.gain(0, 0.25);
    mix1a.gain(1, 0.25);
    mix1a.gain(2, 0.25);
    mix1a.gain(3, 0.25);
    mix1b.gain(0, 0.25);
    mix1b.gain(1, 0.25);
    mix1b.gain(2, 0.25);
    mix1b.gain(3, 0.25);
    mix1c.gain(0, 0.25);
    mix1c.gain(1, 0.25);
    mix1c.gain(2, 0.25);
    mix1c.gain(3, 0.25);
    mix1d.gain(0, 0.25);
    mix1d.gain(1, 0.25);
    mix1d.gain(2, 0.25);
    mix1d.gain(3, 0.25);
    mix2.gain(0, 0.25);
    mix2.gain(1, 0.25);
    mix2.gain(2, 0.25);
    mix2.gain(3, 0.25);
    //reverb.roomsize(0.25);
    //reverb.damping(1);
    synth_set_OSC_A_waveform(WAVEFORM_SINE);
    synth_set_OSC_B_waveform(WAVEFORM_SINE);
    synth_set_OSC_C_waveform(WAVEFORM_SINE);

    synth_set_OSC_A_amplitude(100);
    synth_set_OSC_B_amplitude(100);
    synth_set_OSC_C_amplitude(100);
    synth_set_OSC_D_amplitude(100);

    synth_set_OSC_A_pulseWidth(0);
    synth_set_OSC_B_pulseWidth(0);
    synth_set_OSC_C_pulseWidth(0);

    synth_set_OSC_A_phase(0);
    synth_set_OSC_B_phase(0);
    synth_set_OSC_C_phase(0);
    
    synth_set_envelope_delay(0);
    synth_set_envelope_attack(0);
    synth_set_envelope_hold(0);
    synth_set_envelope_decay(0);
    synth_set_envelope_sustain(100);
    synth_set_envelope_release(0);

    synth_set_OSC_A_freqMult(64);
    synth_set_OSC_B_freqMult(64);
    synth_set_OSC_C_freqMult(64);
}
void synth_set_OSC_A_waveform(byte wf)
{
    oscAwaveform = wf;
    for (int i = 0; i < VOICE_COUNT; i++)
    {
        voices[i].oscA.begin(wf);
    }
}
void synth_set_OSC_B_waveform(byte wf)
{
    oscBwaveform = wf;
    for (int i = 0; i < VOICE_COUNT; i++)
    {
        voices[i].oscB.begin(wf);
    }
}
void synth_set_OSC_C_waveform(byte wf)
{
    oscCwaveform = wf;
    for (int i = 0; i < VOICE_COUNT; i++)
    {
        voices[i].oscC.begin(wf);

        //voices[i].oscD.begin(wf); // experimental
    }
}
void synth_set_OSC_A_pulseWidth(byte value)
{
    oscApulsewidth = value;
    for (int i = 0; i < VOICE_COUNT; i++)
    {
        voices[i].oscA.pulseWidth(value*DIV100);
    }
}
void synth_set_OSC_B_pulseWidth(byte value)
{
    oscBpulsewidth = value;
    for (int i = 0; i < VOICE_COUNT; i++)
    {
        voices[i].oscB.pulseWidth(value*DIV100);
    }
}
void synth_set_OSC_C_pulseWidth(byte value)
{
    oscCpulsewidth = value;
    for (int i = 0; i < VOICE_COUNT; i++)
    {
        voices[i].oscC.pulseWidth(value*DIV100);
    }
}
void synth_set_OSC_A_phase(byte value)
{
    oscAphase = value;
    for (int i = 0; i < VOICE_COUNT; i++)
    {
        voices[i].oscA.phase(value*DIV360BY120);
    }
}
void synth_set_OSC_B_phase(byte value)
{
    oscBphase = value;
    for (int i = 0; i < VOICE_COUNT; i++)
    {
        voices[i].oscB.phase(value*DIV360BY120);
    }
}
void synth_set_OSC_C_phase(byte value)
{
    oscCphase = value;
    for (int i = 0; i < VOICE_COUNT; i++)
    {
        voices[i].oscC.phase(value*DIV360BY120);
    }
}
void synth_set_OSC_A_amplitude(byte value)
{
    oscAamp = value;
    for (int i = 0; i < VOICE_COUNT; i++)
    {
        voices[i].mix.gain(0,value*DIV100);
    }
}
void synth_set_OSC_B_amplitude(byte value)
{
    oscBamp = value;
    for (int i = 0; i < VOICE_COUNT; i++)
    {
        voices[i].mix.gain(1, value*DIV100);
    }
}
void synth_set_OSC_C_amplitude(byte value)
{
    oscCamp = value;
    for (int i = 0; i < VOICE_COUNT; i++)
    {
        voices[i].mix.gain(2, value*DIV100);
    }
}
void synth_set_OSC_D_amplitude(byte value)
{
    oscDamp = value;
    for (int i = 0; i < VOICE_COUNT; i++)
    {
        voices[i].mix.gain(3, value*DIV100);
    }
}
void synth_set_mix1a_gain(byte value)
{
    mix1a_gain = value;
    mix1a.gain(0, value*DIV100);
    mix1a.gain(1, value*DIV100);
    mix1a.gain(2, value*DIV100);
    mix1a.gain(3, value*DIV100);
}
void synth_set_mix1b_gain(byte value)
{
    mix1b_gain = value;
    mix1b.gain(0, value*DIV100);
    mix1b.gain(1, value*DIV100);
    mix1b.gain(2, value*DIV100);
    mix1b.gain(3, value*DIV100);
}
void synth_set_mix1c_gain(byte value)
{
    mix1c_gain = value;
    mix1c.gain(0, value*DIV100);
    mix1c.gain(1, value*DIV100);
    mix1c.gain(2, value*DIV100);
    mix1c.gain(3, value*DIV100);
}
void synth_set_mix1d_gain(byte value)
{
    mix1d_gain = value;
    mix1d.gain(0, value*DIV100);
    mix1d.gain(1, value*DIV100);
    mix1d.gain(2, value*DIV100);
    mix1d.gain(3, value*DIV100);
}
void synth_set_mix2_gain0(byte value)
{
    mix2_gain0 = value;
    mix2.gain(0, value*DIV100);
}
void synth_set_mix2_gain1(byte value)
{
    mix2_gain1 = value;
    mix2.gain(1, value*DIV100);
}
void synth_set_mix2_gain2(byte value)
{
    mix2_gain2 = value;
    mix2.gain(2, value*DIV100);
}
void synth_set_mix2_gain3(byte value)
{
    mix2_gain3 = value;
    mix2.gain(3, value*DIV100);
}

void synth_set_envelope_delay(byte value)
{
    envDelay = value;
    for (int i = 0; i < VOICE_COUNT; i++)
    {
        voices[i].env.delay((float)value*(float)10);
    }
}
void synth_set_envelope_attack(byte value)
{
    envAttack = value;
    for (int i = 0; i < VOICE_COUNT; i++)
    {
        voices[i].env.attack((float)value*(float)10);
    }
}
void synth_set_envelope_hold(byte value)
{
    envHold = value;
    for (int i = 0; i < VOICE_COUNT; i++)
    {
        voices[i].env.hold((float)value*(float)10);
    }
}
void synth_set_envelope_decay(byte value)
{
    envDecay = value;
    for (int i = 0; i < VOICE_COUNT; i++)
    {
        voices[i].env.decay((float)value*(float)10);
    }
}
void synth_set_envelope_sustain(byte value)
{
    envSustain = value;
    for (int i = 0; i < VOICE_COUNT; i++)
    {
        voices[i].env.sustain(value*DIV100);
    }
}
void synth_set_envelope_release(byte value)
{
    envRelease = value;
    for (int i = 0; i < VOICE_COUNT; i++)
    {
        voices[i].env.release((float)value*(float)10);
    }
}
void synth_set_OSC_A_freqMult(byte value)
{
    oscAfreqMult = value;
    
}
void synth_set_OSC_B_freqMult(byte value)
{
    oscBfreqMult = value;
    
}
void synth_set_OSC_C_freqMult(byte value)
{
    oscCfreqMult = value;
    
}