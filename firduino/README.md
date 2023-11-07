# firduino - Arduino FIR Library using ESP-DSP FIR Implementation
# This is a modified version by stojanmar; It includes addition to extend demodulation from SSB to also AM and FM - NFM
# The original source is as described below:

This library uses the [Arduino Audio Tools](https://github.com/pschatzmann/arduino-audio-tools) library and implements a BaseConverter subclass that accepts convolution coefficients for left and right channel. There are two implementations:
- FIRConverter which applies the left and right coefficients to each channel separately
- FIRAddConverter which adds (or subtracts) left from right channel depending on the "direction"

The convolution algoritm is implemented using the [dsps_fir_f32_ae32.S](https://github.com/espressif/esp-dsp/blob/master/modules/fir/float/dsps_fir_f32_ae32.S) from the [esp-dsp](https://github.com/espressif/esp-dsp) library

This implementation is quite efficient and allows (at least) up to 501 taps at a sampling frequency of 44.1kHz on a standard ESP32 running at 240MHz

For an example implementation see [Radio](https://github.com/thaaraak/ESP32-A1S-Tayloe/tree/master/Radio) and the simpler [FIR](https://github.com/thaaraak/ESP32-A1S-Tayloe/tree/master/streams-i2s-fir-i2s)

Note this library only works on the ESP32 platform
