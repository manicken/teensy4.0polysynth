#include <Arduino.h>
#include <Audio.h>

const float noteFreqs[128] = {8.176, 8.662, 9.177, 9.723, 10.301, 10.913, 11.562, 12.25, 12.978, 13.75, 14.568, 15.434, 16.352, 17.324, 18.354, 19.445, 20.602, 21.827, 23.125, 24.5, 25.957, 27.5, 29.135, 30.868, 32.703, 34.648, 36.708, 38.891, 41.203, 43.654, 46.249, 48.999, 51.913, 55, 58.27, 61.735, 65.406, 69.296, 73.416, 77.782, 82.407, 87.307, 92.499, 97.999, 103.826, 110, 116.541, 123.471, 130.813, 138.591, 146.832, 155.563, 164.814, 174.614, 184.997, 195.998, 207.652, 220, 233.082, 246.942, 261.626, 277.183, 293.665, 311.127, 329.628, 349.228, 369.994, 391.995, 415.305, 440, 466.164, 493.883, 523.251, 554.365, 587.33, 622.254, 659.255, 698.456, 739.989, 783.991, 830.609, 880, 932.328, 987.767, 1046.502, 1108.731, 1174.659, 1244.508, 1318.51, 1396.913, 1479.978, 1567.982, 1661.219, 1760, 1864.655, 1975.533, 2093.005, 2217.461, 2349.318, 2489.016, 2637.02, 2793.826, 2959.955, 3135.963, 3322.438, 3520, 3729.31, 3951.066, 4186.009, 4434.922, 4698.636, 4978.032, 5274.041, 5587.652, 5919.911, 6271.927, 6644.875, 7040, 7458.62, 7902.133, 8372.018, 8869.844, 9397.273, 9956.063, 10548.08, 11175.3, 11839.82, 12543.85};
//const float DIV127 = (1.0 / 127.0);

byte oscApitchMult = 64; // set at middle
byte oscBpitchMult = 64; // set at middle
byte oscCpitchMult = 64; // set at middle

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

      //newFreqD = noteFreqs[note]; // experimental
      
      //velocity -= 63; // change of velocity curve
      // newAmp = (float)velocity*DIV64; // change of velocity curve

      newAmp = (float)velocity*(1.0f / 127.0f);
      
      oscA.frequency(GetBendedFreq(oscApitchMult));
      oscB.frequency(GetBendedFreq(oscBpitchMult));
      oscC.frequency(GetBendedFreq(oscCpitchMult));

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
    
    //float newFreqD = 0.0; // experimental

    float GetBendedFreq(byte pitchMult)
    {
        if (pitchMult < 64)
            return noteFreqs[note - 12*(64-pitchMult)];
        else if (pitchMult > 64)
            return noteFreqs[note + 12*(pitchMult-64)];
        else
            return noteFreqs[note];
    }
};