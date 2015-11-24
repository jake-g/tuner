# Tuner

##### Simple guitar tuner. Compiles on OSX and Linux.

### To Compile:
1. Download and install portaudio.
   - For OS X, use homebrew or macports to install -- `brew install portaudio`.
   - For Ubuntu, use `apt-get install portaudio19-dev`.
2. run "make"
3. the output is ./tuner

### Implementation:
This tuner app works by calculating the magnitude of the FFT and mapping it to a note. Many more ideal methods could be pursued.

#### Copyright:
Modified version by Jake Garrison (2014) <br>
Original concept by Bjorn Roche (2012) <br>
