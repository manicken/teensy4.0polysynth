<!DOCTYPE html>
<html>
<head>
<meta name="viewport" content="width=device-width, initial-scale=1.0">
</head>
<body>

<h1>Teensy4polysynth</h1>

<picture>
  <source media="(min-width: 650px)" srcset="https://github.com/manicken/teensy4.0polysynth/blob/master/IMG_20200518_211540.jpg">
  <source media="(min-width: 465px)" srcset="https://github.com/manicken/teensy4.0polysynth/blob/master/IMG_20200518_211540.jpg">
  <img src="https://github.com/manicken/teensy4.0polysynth/blob/master/IMG_20200518_211540.jpg" alt="mainPic" style="width:auto;">
</picture>

<p>teensy 4.0 synth with class containing one voice, the only thing that needs object-pointers is the AudioConnection.</p>

<p>each voice contains 3waveform oscillators</p>

<p>is uses a cheap I2S compatible PT8211 DAC, that is connected to the second i2s port of teensy 4 (this port is pin-to-pin compatible with PT8211 so that no complicated routing is needed and the wiring can be short as possible, the only wire that needs to be longer is the vcc line, but with good decoupling this can be fixed. There is some noice issues coming from the usb-connection, but they are gone by powering by a powerbank.</p>
<p>There is also an MAX6818 switch-debouncer that is used for the different piano foot pedals, currently only a sustain-"pedal" is connected.</p>

<p><strong></strong></p>

</body>
</html>
