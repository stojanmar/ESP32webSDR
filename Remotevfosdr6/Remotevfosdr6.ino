
/*Za kontrolo radijske postaje preko Wi Fija
 * ver02 dodam serijsko komunikacijo za kontrolo v esp32
 * ver03 dodam izklop močnostne na audiokitu zaradi EMC
 * ver05 ima antena tune po polinomu 3. stopnje ne dela točno po excell formuli!!!!
 * ver6 naredim raje z interpolacijo
 * dela na nodemcu in upravlja Elektor SDR frontend
 * var01 3.1.21
 * created by S52UV
 */

#include <Arduino.h>
#include <ESP8266WiFi.h>
#include <ESPAsyncTCP.h>
#include <ESP8266mDNS.h>
#include <WiFiClient.h>
#include <ESP8266WiFiMulti.h>
//#include <ESP8266WebServer.h>
#include <WiFiUdp.h>
#include <ArduinoOTA.h>
#include <SoftwareSerial.h>
#include <ESPAsyncWebServer.h>
#include <si5351.h>
#include <Wire.h>
#include "InterpolationLib.h"

#define SI5351_REF     25000000UL  //change this to the frequency of the crystal on your si5351’s PCB

#define BAUD_RATE 9600
#ifndef STASSID
#define STASSID "XXXXXXX"
#define STAPSK  "xxxxxxx"
#endif
#define DELAY_BETWEEN_COMMANDS 1000
#define CORRECTION 162962 // See how to calibrate your Si5351A (0 if you do not want).
#define PWMpin D7   //ta pin povezati na vezje za varikap diodo


// The Si5351 instance.
Si5351 si5351;


const char* ssid = STASSID;
const char* password = STAPSK;

const char* PARAM_INPUT_1 = "input1";
const char* PARAM_INPUT_2 = "input2";
const char* PARAM_INPUT_3 = "input3";
const char* PARAM_INPUT_4 = "input4";
const char* PARAM_INPUT_5 = "input5";
const char* PARAM_INPUT_6 = "input6";
const char* PARAM_INPUT_7 = "input7";

String frekvenca = "200000000";  //do 200Mhz gre
String stepinc = "1000000"; //bi bil increment 1Mhz oz. 250Hz pri SDR
//String izhodonoff = "Off"; // On or Off
//String inString = "";    // string to hold input
double frekvin = 3605000 * 4; // 1xHz
double stepin = 1000 * 4; // za 1kHz
int pwmduty = 1023; // od0 do 1023 za pwm na pinu D7 GPIO13
//int pwmdutyt = 1023; // samo za testiranje formule
int x = 3605;   //spremenljivka za v formulo bo f deljena z 1000
int zz = 54;
int uu = 0;
int vv = 0;
char buf1[10] = "00360500";

const int numValues = 14;
double xValues[14] = {   3720,  3750,  3800,  3950,  4500,  5000,  5500,  6000,  7100, 8000, 9600, 10700, 11700, 13800 };
double yValues[14] = { 1023, 1010, 1000, 965, 830, 725, 640, 570, 460, 385, 260, 200, 150, 0 };

String readfrekvenca() {
  int webfrekvenca = frekvin / 4;  // nazaj v readable frekvenco
    Serial.println(webfrekvenca); 
  return String(webfrekvenca);
}

// HTML web page to handle 7 input fields (input1, input2, input3,....)
const char index_html[] PROGMEM = R"rawliteral(
<!DOCTYPE HTML><html><head>
  <title>Esp32SDR Input Form</title>
  <meta name="viewport" content="width=device-width, initial-scale=1">
  </head><body>
  <style>
body {
   background-color: Orange;
   font-family: Arial, Helvetica, sans-serif;
}
h1 {
   color: navy;
   margin-left: 20px;
   font-size: 150%
}
meter {
  width: 240px;
  height: 25px; 
}
input[type=submit] {
  width: 80px;
  height: 33px;
  background-color: #128F76;
  border: none;
  color: white;
  text-align: center;
  text-decoration: none;
  display: inline-block;
  font-size: 16px;
  margin: 4px 2px;
  cursor: pointer;
  border-radius: 5px;
}
input[type="text"] {
   margin: 0;
   height: 28px;
   background: white;
   font-size: 16px;
   width: 120px;
   appearance: none;
   box-shadow: none;
   border-radius: 5px;
   -webkit-border-radius: 5px;
   -moz-border-radius: 5px;
   -webkit-appearance: none;
   border: 1px solid black;
   border-radius: 10px;
}
</style>
<center>
  <h1>** S52UV ESP32SDR Radio **</h1>
  <p>Signal: <meter  max="100" value="20" align="left"></meter></p>
  <form action="/get">
    Frekvenca : <input type="text" name="input1">
    <input type="submit" value="Submit">
  </form>
  <form action="/get">
    Stepincdec: <input type="text" name="input2" value="1000">
    <input type="submit" value="Submit">
  </form>
  <form action="/get">
    Anten tune: <input type="text" name="input3" value="1023">
    <input type="submit" value="Submit">
  </form>
  <form action="/get">
    Makestep+: <input type="text" name="input4" value="Up">
    <input type="submit" value="Submit">
  </form>
  <form action="/get">
    Makestep -: <input type="text" name="input5" value="Down">
    <input type="submit" value="Submit">
  </form>
  <form action="/get">
    Modulation: <input type="text" name="input6" value="L">
    <input type="submit" value="Submit">
  </form>
  <form action="/get">
    Volume 0..7 <input type="text" name="input7" value="4">
    <input type="submit" value="Submit">
  </form>
  <br>
   <br>
   <span id="resultstr">%FREKVENCA%</span>
   <input type="text" width="200px" size="31" id="resultstr" placeholder="frekvenca v Hz"><br>
   <p>Freq and step in Hz, Step = Up or Down</p>
   <p>Stran kreirana Dec 2022 : stojanmar </p><br>
  </center>
</body>
<script>
setInterval(function ( ) {
  var xhttp = new XMLHttpRequest();
  xhttp.onreadystatechange = function() {
    if (this.readyState == 4 && this.status == 200) {
      document.getElementById("resultstr").innerHTML = this.responseText;
    }
  };
  xhttp.open("GET", "/resultstr", true);
  xhttp.send();
}, 2000) ;
</script>
</html>)rawliteral";

void notFound(AsyncWebServerRequest *request) {
  request->send(404, "text/plain", "Not found");
}

ESP8266WiFiMulti wifiMulti;     // Create an instance of the ESP8266WiFiMulti class, called 'wifiMulti'
AsyncWebServer server(80);
//ESP8266WebServer server(80);
// Reminder: the buffer size optimizations here, in particular the isrBufSize that only accommodates
// a single 8N1 word, are on the basis that any char written to the loopback SoftwareSerial adapter gets read
// before another write is performed. Block writes with a size greater than 1 would usually fail. 
//SoftwareSerial swSer(D5, D6, false, 64);  //D5isRx  D6 is Tx
//SoftwareSerial swSer;  //D5isRx  D6 is Tx
SoftwareSerial swSer;
//const int led = BUILTIN_LED;

String processor(const String& var){
  //Serial.println(var);
  if(var == "FREKVENCA"){
    return readfrekvenca();
  }
  return String();
}

void setup() {
  
  analogWrite(PWMpin, 1023);
  analogWriteRange(1023);   //range for pwm
  analogWriteFreq(5000);    //frekvenca pwmja v Hz
  
  Serial.begin(9600);
  swSer.begin(BAUD_RATE, SWSERIAL_8N1, D5, D6, false, 95, 11);
  pinMode(D6,OUTPUT_OPEN_DRAIN); // dodam, da naredim TX output open drain
  //swSer.enableTxGPIOOpenDrain(1);  // dodam, da naredim output open drain
  
 
  // Set your Static IP address
  IPAddress local_IP(192, 168, 1, xxx);
  // Set your Gateway IP address
  IPAddress gateway(192, 168, 1, 1);

  IPAddress subnet(255, 255, 0, 0);
  IPAddress primaryDNS(8, 8, 8, 8);   //optional
  IPAddress secondaryDNS(8, 8, 4, 4); //optional
   // Configures static IP address
  if (!WiFi.config(local_IP, gateway, subnet, primaryDNS, secondaryDNS)) {
    Serial.println("StaticIP Failed to configure");
      }
  wifiMulti.addAP("XXXXXXX", password);   // add Wi-Fi networks you want to connect to
  wifiMulti.addAP("YYYYYYY", password);
  //wifiMulti.addAP("ssid_from_AP_3", "your_password_for_AP_3");
  Serial.println("Connecting");
  int i = 0;
  while (wifiMulti.run() != WL_CONNECTED) { // Wait for the Wi-Fi to connect: scan for Wi-Fi networks, and connect to the strongest of the networks above
    delay(250);
    Serial.print('.');
  }
  Serial.println('\n');
  Serial.print("Connected to ");
  Serial.println(WiFi.SSID());              // Tell us what network we're connected to
  Serial.print("IP address:\t");
  Serial.println(WiFi.localIP());           // Send the IP address of the ESP8266 to the computer

    if (MDNS.begin("esp8266")) {
    Serial.println("MDNS Responder Started");
  }

  // Send web page with input fields to client
  server.on("/", HTTP_GET, [](AsyncWebServerRequest *request){
    request->send_P(200, "text/html", index_html, processor);
  });
  server.on("/resultstr", HTTP_GET, [](AsyncWebServerRequest *request){
    request->send_P(200, "text/plain", readfrekvenca().c_str());
  });

  // Send a GET request to <ESP_IP>/get?input1=<inputMessage>
  server.on("/get", HTTP_GET, [] (AsyncWebServerRequest *request) {
    String inputMessage;
    String inputParam;
    // GET input1 value on <ESP_IP>/get?input1=<inputMessage>
    if (request->hasParam(PARAM_INPUT_1)) {
      inputMessage = request->getParam(PARAM_INPUT_1)->value();
      inputParam = PARAM_INPUT_1;
      frekvenca = inputMessage;
      frekvin = 4 * frekvenca.toInt(); // morda funkcija atol() tudi dela
      //si5351.set_freq(frekvin * 100, SI5351_CLK0);
      si5351.set_freq(frekvin * 100, SI5351_CLK1);

      x = (int) (inputMessage.toInt() / 1000);
      x = constrain(x, 3720, 13800);    // omejim za delovanje formule
      pwmduty = (int) (Interpolation::Linear(xValues, yValues, numValues, x, true)); 
                  
      analogWrite(PWMpin, pwmduty);         
    }
    // GET input2 value on <ESP_IP>/get?input2=<inputMessage>
    else if (request->hasParam(PARAM_INPUT_2)) {
      inputMessage = request->getParam(PARAM_INPUT_2)->value();
      inputParam = PARAM_INPUT_2;
      stepinc = inputMessage;
      stepin = 4 * stepinc.toInt();
            
    }
    // GET input3 value on <ESP_IP>/get?input3=<inputMessage>
    else if (request->hasParam(PARAM_INPUT_3)) {
      inputMessage = request->getParam(PARAM_INPUT_3)->value();
      inputParam = PARAM_INPUT_3;
      
      pwmduty = inputMessage.toInt();
      // tukaj preracunaj s formulo
      analogWrite(PWMpin, pwmduty);
        
    }
    // GET input4 value on <ESP_IP>/get?input4=<inputMessage>
    else if (request->hasParam(PARAM_INPUT_4)) {
      inputMessage = request->getParam(PARAM_INPUT_4)->value();
      inputParam = PARAM_INPUT_4;
      if (inputMessage == "Up") {frekvin = frekvin + stepin;} // povišaj frekvenco     
      //si5351.set_freq(frekvin * 100, SI5351_CLK0);
      si5351.set_freq(frekvin * 100, SI5351_CLK1);

      x = (int) (frekvin / 4000);
      x = constrain(x, 3720, 13800);    // omejim za delovanje formule
      pwmduty = (int) (Interpolation::Linear(xValues, yValues, numValues, x, true));             
      analogWrite(PWMpin, pwmduty); 
    }
    // GET input5 value on <ESP_IP>/get?input5=<inputMessage>
    else if (request->hasParam(PARAM_INPUT_5)) {
      inputMessage = request->getParam(PARAM_INPUT_5)->value();
      inputParam = PARAM_INPUT_5;
      if (inputMessage == "Down") {frekvin = frekvin - stepin;} // znizaj
      //si5351.set_freq(frekvin * 100, SI5351_CLK0);
      si5351.set_freq(frekvin * 100, SI5351_CLK1);

      x = (int) (frekvin / 4000);
      x = constrain(x, 3720, 13800);    // omejim za delovanje formule
      pwmduty = (int) (Interpolation::Linear(xValues, yValues, numValues, x, true));             
      analogWrite(PWMpin, pwmduty); 
    }
    // GET input6 value on <ESP_IP>/get?input6=<inputMessage>
    else if (request->hasParam(PARAM_INPUT_6)) {
      inputMessage = request->getParam(PARAM_INPUT_6)->value();
      inputParam = PARAM_INPUT_6;
      if (inputMessage == "L") {swSer.println("L");} // LSB mode
      if (inputMessage == "U") {swSer.println("U");} // USB mode
      if (inputMessage == "A") {swSer.println("A");} // AM mode
      if (inputMessage == "F") {swSer.println("F");} // FM-N mode
      /*if (modulacija == "W") {xx=7;} // FM-W mode
      if (modulacija == "C") {xx=2;} // CW mode*/ 
    }
    // GET input7 value on <ESP_IP>/get?input7=<inputMessage>
    else if (request->hasParam(PARAM_INPUT_7)) {
      inputMessage = request->getParam(PARAM_INPUT_7)->value();
      inputParam = PARAM_INPUT_7;

      swSer.println("V" + inputMessage);  // posredujem v drugi board
      /*svolume = inputMessage;
      volume = svolume.toInt(); // morda funkcija atol() tudi dela
      audio.setVolume(volume); // 0...21*/  
    }
    
    else {
      inputMessage = "No message sent";
      inputParam = "none";
    }
    
      request->send_P(200, "text/html", index_html);                                                                
  });
  server.onNotFound(notFound);
  server.begin();
  Serial.println("HTTP Server Started");

  // Initiating the Signal Generator (si5351)
  si5351.init(SI5351_CRYSTAL_LOAD_8PF, SI5351_REF, CORRECTION);
  
  si5351.set_freq(frekvin * 100, SI5351_CLK1);
  // Now we can set CLK1 to have a 90 deg phase shift by entering
  // 50 in the CLK1 phase register, since the ratio of the PLL to
  // the clock frequency is 50.
  //si5351.set_phase(SI5351_CLK0, 0);
  //si5351.set_phase(SI5351_CLK1, 50); tko jih das za 90 deg
  si5351.set_phase(SI5351_CLK1, 0);
  // We need to reset the PLL before they will be in phase alignment
  si5351.pll_reset(SI5351_PLLA);
  //si5351.drive_strength(SI5351_CLK0, SI5351_DRIVE_8MA); // Set for max power default is 2MA
  //si5351.set_clock_pwr(SI5351_CLK0, 0); // Disable the clock initially
  // To see later
  // si5351.set_freq(bfoFreq, SI5351_CLK1);
  //si5351.output_enable(SI5351_CLK1, 0);
  si5351.output_enable(SI5351_CLK2, 0);  // ugasnem tetji izhod

  si5351.update_status();
  // Show the initial system information
  Serial.println("Module intializated");

  ArduinoOTA.onStart([]() {
    String type;
    if (ArduinoOTA.getCommand() == U_FLASH) {
      type = "sketch";
    } else { // U_SPIFFS
      type = "filesystem";
    }

    // NOTE: if updating SPIFFS this would be the place to unmount SPIFFS using SPIFFS.end()
    Serial.println("Start updating " + type);
  });
  ArduinoOTA.onEnd([]() {
    Serial.println("\nEnd");
  });
  ArduinoOTA.onProgress([](unsigned int progress, unsigned int total) {
    Serial.printf("Progress: %u%%\r", (progress / (total / 100)));
  });
  ArduinoOTA.onError([](ota_error_t error) {
    Serial.printf("Error[%u]: ", error);
    if (error == OTA_AUTH_ERROR) {
      Serial.println("Auth Failed");
    } else if (error == OTA_BEGIN_ERROR) {
      Serial.println("Begin Failed");
    } else if (error == OTA_CONNECT_ERROR) {
      Serial.println("Connect Failed");
    } else if (error == OTA_RECEIVE_ERROR) {
      Serial.println("Receive Failed");
    } else if (error == OTA_END_ERROR) {
      Serial.println("End Failed");
    }
  });
  ArduinoOTA.begin();
  Serial.println("Ready");
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());
}

void loop() {
  ArduinoOTA.handle();
  //delay(1000);
  //digitalWrite(D0, HIGH); // Turn ON LED
  //delay(500); 
}
