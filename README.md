# ESP32webSDR
Standalone SDR with esp32 and esp8266
This project includes esp32-A1S as the core for SDR functions for the receiver with SSS, AM, NFM demodulation. To control it another esp8266-Nodemcu is used to provide the webserver with possibilty to set receiving frequency, frequency step UP and DOWN, audio volume control, type of demodulation and more. The code is written for the Arduino IDE.

I have got a basic inspiration from the https://github.com/thaaraak/ESP32-A1S-Tayloe . His excelent work was related for use of another grate audio tools library created by Phil Schatzmann https://github.com/pschatzmann/arduino-audio-tools . From this two I used their libraries and create my modified version which extends SSB demodulation mode with AM and NFW.
For the HW part of the Tayloe mixer I used Elektor SDR board normaly intended to be connected to inputs of the PC sound card and modified it to improove the sensitivity on RF input.
Special care was also taken about reducing the EMI noise, so none switching power supplies in complete radio is in use. Also care about the grounding connections and decopling capacitors allowed me to make SDR radio without having specific PCB designed for assembly. Supprisingly with this care even the WiFi operation of the esp8266 does not interfere with reception of this radio.
For the LO with 4xfrequency necessary for thr Tayloe operation the popular Si5351 is used with Arduino library for it.
There are several ideas not completed yet, therefore I declare this project as still in progress and SW code versions might follow soon.

The documentation will be created rather later in progress as several changes are under tests of the currenty prototype build. For now only a Youtube video link here ........
