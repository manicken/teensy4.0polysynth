#include <Arduino.h>
#include <Audio.h>
#include <Wire.h>
#include <SPI.h>
#include <SD.h>
#include <SerialFlash.h>
#include <MIDI.h>

AudioOutputPT8211_2           i2s1;           //xy=1049,465

const float noteFreqs[128] = {8.176, 8.662, 9.177, 9.723, 10.301, 10.913, 11.562, 12.25, 12.978, 13.75, 14.568, 15.434, 16.352, 17.324, 18.354, 19.445, 20.602, 21.827, 23.125, 24.5, 25.957, 27.5, 29.135, 30.868, 32.703, 34.648, 36.708, 38.891, 41.203, 43.654, 46.249, 48.999, 51.913, 55, 58.27, 61.735, 65.406, 69.296, 73.416, 77.782, 82.407, 87.307, 92.499, 97.999, 103.826, 110, 116.541, 123.471, 130.813, 138.591, 146.832, 155.563, 164.814, 174.614, 184.997, 195.998, 207.652, 220, 233.082, 246.942, 261.626, 277.183, 293.665, 311.127, 329.628, 349.228, 369.994, 391.995, 415.305, 440, 466.164, 493.883, 523.251, 554.365, 587.33, 622.254, 659.255, 698.456, 739.989, 783.991, 830.609, 880, 932.328, 987.767, 1046.502, 1108.731, 1174.659, 1244.508, 1318.51, 1396.913, 1479.978, 1567.982, 1661.219, 1760, 1864.655, 1975.533, 2093.005, 2217.461, 2349.318, 2489.016, 2637.02, 2793.826, 2959.955, 3135.963, 3322.438, 3520, 3729.31, 3951.066, 4186.009, 4434.922, 4698.636, 4978.032, 5274.041, 5587.652, 5919.911, 6271.927, 6644.875, 7040, 7458.62, 7902.133, 8372.018, 8869.844, 9397.273, 9956.063, 10548.08, 11175.3, 11839.82, 12543.85};
const float DIV127 = (1.0 / 127.0);
const float DIV64 = (1.0/64.0);

byte oscAwaveform = WAVEFORM_SINE;
byte oscBwaveform = WAVEFORM_SINE;
byte oscCwaveform = WAVEFORM_SINE;

byte oscAamp = 127;
byte oscBamp = 127;
byte oscCamp = 127;

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
byte envSustain = 127;
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
    //AudioSynthWaveform oscDA; // experimental
    //AudioSynthWaveform oscDB; // experimental
    //AudioSynthWaveformModulated oscD; // experimental

    AudioMixer4 mix;
    AudioEffectEnvelope env;

    AudioConnection *pcOscA;
    AudioConnection *pcOscB;
    AudioConnection *pcOscC;
    //AudioConnection *pcOscDA; // experimental
    //AudioConnection *pcOscDB; // experimental
    //AudioConnection *pcOscD; // experimental

    AudioConnection *patchCordMix;
    
    Voice()
    {
      pcOscA = new AudioConnection(oscA, 0, mix, 0);
      pcOscB = new AudioConnection(oscB, 0, mix, 1);
      pcOscC = new AudioConnection(oscC, 0, mix, 2);

      //pcOscDA = new AudioConnection(oscA, 0, oscD, 0); // experimental
      //pcOscDB = new AudioConnection(oscB, 0, oscD, 1); // experimental
      //pcOscD = new AudioConnection(oscD, 0, mix, 3); // experimental

      patchCordMix = new AudioConnection(mix, 0, env, 0);
    }
    void noteOn(byte Note, byte velocity)
    {
      note = Note;
      newFreqA = GetNewFreq(oscAfreqMult);
      newFreqB = GetNewFreq(oscBfreqMult);
      newFreqC = GetNewFreq(oscCfreqMult);
      //newFreqD = noteFreqs[note]; // experimental
      
      //velocity -= 63;
      newAmp = (float)velocity*DIV127;
      
      oscA.frequency(newFreqA);
      oscB.frequency(newFreqB);
      oscC.frequency(newFreqC);
      //oscD.frequency(newFreqD); // experimental
      
      oscA.amplitude(newAmp);
      oscB.amplitude(newAmp);
      oscC.amplitude(newAmp);
      //oscD.amplitude(newAmp); // experimental

      env.noteOn();
    }
    void noteOff(byte velocity)
    {
      env.noteOff();
      note = 0;
    }
    byte note = 0;
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

Voice voices[] = {Voice(), Voice(), Voice(), Voice(), Voice(), Voice(), Voice(), Voice()};

void noteOn(byte note, byte velocity)
{
    digitalWrite(ledPin, HIGH);
    for (int i = 0; i < 8; i++) // first failsafe check 
    {
        if (voices[i].note == note)
        {
            return; // the key just glitched, ignore.
        }
    }
    for (int i = 0; i < 8; i++) // second see if there is any free "spot"
    {
        if (voices[i].note == 0 && !voices[i].env.isActive())
        {
            voices[i].noteOn(note, velocity);
            return;
        }
    }
}
void noteOff(byte note, byte velocity)
{
    digitalWrite(ledPin, LOW);
    for (int i = 0; i < 8; i++)
    {
        if (voices[i].note == note)
        {
            voices[i].noteOff(velocity);
            return;
        }
    }
}

AudioMixer4 mix1a;
AudioMixer4 mix1b;
AudioMixer4 mix2;
//AudioEffectFreeverb reverb;

AudioConnection patchCordVoice1(voices[0].env, 0, mix1a , 0);
AudioConnection patchCordVoice2(voices[1].env, 0, mix1a , 1);
AudioConnection patchCordVoice3(voices[2].env, 0, mix1a , 2);
AudioConnection patchCordVoice4(voices[3].env, 0, mix1a , 3);
AudioConnection patchCordVoice5(voices[4].env, 0, mix1b , 0);
AudioConnection patchCordVoice6(voices[5].env, 0, mix1b , 1);
AudioConnection patchCordVoice7(voices[6].env, 0, mix1b , 2);
AudioConnection patchCordVoice8(voices[7].env, 0, mix1b , 3);

AudioConnection patchCordMix1a(mix1a, 0, mix2, 0);
AudioConnection patchCordMix1b(mix1b, 0, mix2, 1);
//AudioConnection pc1(mix2, 0, reverb, 0);

AudioConnection patchCordOutL(mix2, 0, i2s1, 0);
AudioConnection patchCordOutR(mix2, 0, i2s1, 1);

void synth_Init(void)
{
    mix1a.gain(0, 0.25);
    mix1a.gain(1, 0.25);
    mix1a.gain(2, 0.25);
    mix1a.gain(3, 0.25);
    mix1b.gain(0, 0.25);
    mix1b.gain(1, 0.25);
    mix1b.gain(2, 0.25);
    mix1b.gain(3, 0.25);
    mix2.gain(0, 0.5);
    mix2.gain(1, 0.5);
    //reverb.roomsize(0.25);
    //reverb.damping(1);
    synth_set_OSC_A_waveform(WAVEFORM_SINE);
    synth_set_OSC_B_waveform(WAVEFORM_SINE);
    synth_set_OSC_C_waveform(WAVEFORM_SINE);

    synth_set_OSC_A_amplitude(127);
    synth_set_OSC_B_amplitude(127);
    synth_set_OSC_C_amplitude(127);

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
    synth_set_envelope_sustain(127);
    synth_set_envelope_release(0);

    synth_set_OSC_A_freqMult(64);
    synth_set_OSC_B_freqMult(64);
    synth_set_OSC_C_freqMult(64);
}
void synth_set_OSC_A_waveform(byte wf)
{
    oscAwaveform = wf;
    for (int i = 0; i < 8; i++)
    {
        voices[i].oscA.begin(wf);
    }
}
void synth_set_OSC_B_waveform(byte wf)
{
    oscBwaveform = wf;
    for (int i = 0; i < 8; i++)
    {
        voices[i].oscB.begin(wf);
    }
}
void synth_set_OSC_C_waveform(byte wf)
{
    oscCwaveform = wf;
    for (int i = 0; i < 8; i++)
    {
        voices[i].oscC.begin(wf);
        //voices[i].oscD.begin(wf); // experimental
    }
}
void synth_set_OSC_A_pulseWidth(byte value)
{
    oscApulsewidth = value;
    for (int i = 0; i < 8; i++)
    {
        voices[i].oscA.pulseWidth((float)value/(float)127);
    }
}
void synth_set_OSC_B_pulseWidth(byte value)
{
    oscBpulsewidth = value;
    for (int i = 0; i < 8; i++)
    {
        voices[i].oscB.pulseWidth((float)value/(float)127);
    }
}
void synth_set_OSC_C_pulseWidth(byte value)
{
    oscCpulsewidth = value;
    for (int i = 0; i < 8; i++)
    {
        voices[i].oscC.pulseWidth((float)value/(float)127);
    }
}
void synth_set_OSC_A_phase(byte value)
{
    oscAphase = value;
    for (int i = 0; i < 8; i++)
    {
        voices[i].oscA.phase(((float)value/(float)127)*360);
    }
}
void synth_set_OSC_B_phase(byte value)
{
    oscBphase = value;
    for (int i = 0; i < 8; i++)
    {
        voices[i].oscB.phase(((float)value/(float)127)*360);
    }
}
void synth_set_OSC_C_phase(byte value)
{
    oscCphase = value;
    for (int i = 0; i < 8; i++)
    {
        voices[i].oscC.phase(((float)value/(float)127)*360);
    }
}
void synth_set_OSC_A_amplitude(byte value)
{
    oscAamp = value;
    for (int i = 0; i < 8; i++)
    {
        voices[i].mix.gain(0,(float)value/(float)127);
    }
}
void synth_set_OSC_B_amplitude(byte value)
{
    oscBamp = value;
    for (int i = 0; i < 8; i++)
    {
        voices[i].mix.gain(1, (float)value/(float)127);
    }
}
void synth_set_OSC_C_amplitude(byte value)
{
    oscCamp = value;
    for (int i = 0; i < 8; i++)
    {
        voices[i].mix.gain(2, (float)value/(float)127);
    }
}
void synth_set_envelope_delay(byte value)
{
    envDelay = value;
    for (int i = 0; i < 8; i++)
    {
        voices[i].env.delay((float)value*(float)10);
    }
}
void synth_set_envelope_attack(byte value)
{
    envAttack = value;
    for (int i = 0; i < 8; i++)
    {
        voices[i].env.attack((float)value*(float)10);
    }
}
void synth_set_envelope_hold(byte value)
{
    envHold = value;
    for (int i = 0; i < 8; i++)
    {
        voices[i].env.hold((float)value*(float)10);
    }
}
void synth_set_envelope_decay(byte value)
{
    envDecay = value;
    for (int i = 0; i < 8; i++)
    {
        voices[i].env.decay((float)value*(float)10);
    }
}
void synth_set_envelope_sustain(byte value)
{
    envSustain = value;
    for (int i = 0; i < 8; i++)
    {
        voices[i].env.sustain((float)value/(float)127);
    }
}
void synth_set_envelope_release(byte value)
{
    envRelease = value;
    for (int i = 0; i < 8; i++)
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