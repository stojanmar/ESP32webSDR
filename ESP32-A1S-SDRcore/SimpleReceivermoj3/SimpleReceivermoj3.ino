// izberi board ESP32 Dev Module  dela na ESP32-A1S
#include "Bounce2.h"
//#include "si5351.h"
//#include "Encoder.h"
//rev03 dodam AM in FM demodulacijo in moje filtre

#include "AudioTools.h"
#include "es8388.h"

//#include "FIRConverter.h"
#include "Wire.h"
#include "WiFi.h"
//#include "LiquidCrystal_I2C.h"

//#include "fir_coeffs_501Taps_44100_150_4000.h"
//#include "fir_coeffs_251Taps_22000_350_6000.h"
//#include "fir_coeffs_501Taps_22000_350_10000.h"
//#include "fir_coeffs_161Taps_44100_200_19000.h"
//#include "fir_coeffs_251Taps_44100_500_21000.h"

// Lowpass coefficients for 44.1 kHz
#include "coefs241taps44100_100_21000.h"
#include "coef141TapLPF44100_0_8000.h" 
#include "120-tap-4khz-lowpass.h"
#include "FIRConverter.h"
#include "ChannelAddConverter.h"

//#define BUTTON_PIN_1 5  // this is KEY6
#define BUTTON_PIN_1 13  // this is KEY2
//#define BUTTON_PIN_2 18
//#define BUTTON_PIN_3 23
#define AF_OUTPUT 21 // pin for AF Amplifiers enable.
// Instantiate a Bounce objects
Bounce debouncer1 = Bounce(); 
//Bounce debouncer2 = Bounce(); 
//Bounce debouncer3 = Bounce();

uint16_t sample_rate = 44100;
uint16_t channels = 2;
uint16_t bits_per_sample = 16;

I2SStream i2s;
StreamCopy copier(i2s, i2s); // copies sound into i2s
FIRDemConverter<int16_t> *fir;
FIRDemConverter<int16_t> *firam;
FIRConverter<int16_t> *lowpass_fir;
MultiConverter<int16_t> *multi;

int lastMult = -1;
int currentFrequency = 5000000;
float currentDir = 1.0;
//float glasnost = 0.5;

//Bounce bounce = Bounce();
int directionState = 1;

es8388 codec;   // je bila v seetup I2S

//Si5351 *si5351;
TwoWire wire(0);
TwoWire externalWire(1);

void setupI2S()
{
  // Input/Output Modes
  es_dac_output_t output = (es_dac_output_t) ( DAC_OUTPUT_LOUT1 | DAC_OUTPUT_LOUT2 | DAC_OUTPUT_ROUT1 | DAC_OUTPUT_ROUT2 );
  es_adc_input_t input = ADC_INPUT_LINPUT2_RINPUT2;
  //  es_adc_input_t input = ADC_INPUT_LINPUT1_RINPUT1;

  //es8388 codec;

  codec.begin( &wire );
  codec.config( bits_per_sample, output, input, 90 );

  // start I2S in
  Serial.println("starting I2S...");
  auto config = i2s.defaultConfig(RXTX_MODE);
  config.sample_rate = sample_rate; 
  config.bits_per_sample = bits_per_sample; 
  config.channels = 2;
  config.i2s_format = I2S_STD_FORMAT;
  config.pin_ws = 25;
  config.pin_bck = 5;   // bilo je 27 za drugi modul
  config.pin_data = 26;
  config.pin_data_rx = 35;
  config.pin_mck = 0;

  fir->setGain(8);   //prej je bil gain 4
  i2s.begin(config);
}

void setupFIR()
{
  multi = new MultiConverter<int16_t>();
  //fir = new FIRAddConverter<int16_t>( (float*)&coeffs_hilbert_251Taps_44100_500_21000, (float*)&coeffs_delay_251, 251 );
  //fir = new FIRAddConverter<int16_t>( (float*)&coef241Tapsplus45_44100_100_21000, (float*)&coef241Tapsminus45_44100_100_21000, 241 );
  //fir = new FIRDemConverter<int16_t>( (float*)&coef241Tapsplus45_44100_100_21000, (float*)&coef241Tapsminus45_44100_100_21000, 241 );
  //fir = new FIRAddConverter<int16_t>( (float*)&coeffs_hilbert_501Taps_22000_350_10000, (float*)&coeffs_delay_501, 501 );
  //fir = new FIRAddConverter<int16_t>( (float*)&coeffs_hilbert_501Taps_44100_350_10000, (float*)&coeffs_delay_501, 501 );
    fir = new FIRDemConverter<int16_t>( (float*)&coef241Tapsplus45_44100_100_21000, (float*)&coef241Tapsminus45_44100_100_21000, 241 );
    //firam = new FIRDemConverter<int16_t>( (float*)&coef241Tapsplus45_44100_100_21000, (float*)&coef241Tapsplus45_44100_100_21000, 241 );
    firam = new FIRDemConverter<int16_t>( (float*)&coef141TapLPF44100_0_8000, (float*)&coef141TapLPF44100_0_8000, 141 );
    //firam = new FIRDemConverter<int16_t>( (float*)&lowpass_4KHz, (float*)&lowpass_4KHz, 120 );

  fir->setGain(1);
  fir->setDirection(directionState );
  fir->setCorrection(currentDir);
  firam->setGain(1);
  firam->setDirection(directionState );
  firam->setCorrection(currentDir);
  
  lowpass_fir = new FIRConverter<int16_t>( (float*)&lowpass_4KHz, (float*)&lowpass_4KHz, 120 );
  
  multi->add( *fir );
  multi->add( *lowpass_fir );
}

void setupFIRchg()
{
  //~FIRConverter();
  //delete[] fir;
  //fir = delete FIRDemConverter<int16_t>();
  //delete[] multi;
  //delete[] MultiConverter<int16_t>();
  multi = new MultiConverter<int16_t>();

  if (directionState == 1 || directionState == 2) {
        fir->setDirection(directionState );
        multi->add( *fir );
        multi->add( *lowpass_fir );
      }
  
  if (directionState == 3 || directionState == 4){
        firam->setDirection(directionState );
        multi->add( *firam );
        multi->add( *lowpass_fir );
      }
}

void setup() 
{

  Serial.begin(115200);
    while(!Serial);
  AudioLogger::instance().begin(Serial, AudioLogger::Error);
/* UART1  -> Serial1
  * RX Pin -> GPIO 14
  * TX Pin -> GPIO 12
  * UART Configuration -> SERIAL_8N1*/
  Serial1.begin(9600,SERIAL_8N1,14,12); //Rx je 14  TX je 12
  //Serial1.println("Hello from UART1");
  //wire.setPins( 33, 32 );
  wire.setPins( 18, 23 );  // DATA, CLK
 // Setup the first button with an internal pull-up :
  pinMode(BUTTON_PIN_1,INPUT_PULLUP);
  // After setting up the button, setup the Bounce instance :
  debouncer1.attach(BUTTON_PIN_1);
  debouncer1.interval(10); // interval in ms


  //wireExt.setPins( EXT_SDA, EXT_SCL );
  
  //changeFrequency(14200000);  
  pinMode(AF_OUTPUT,OUTPUT);  // AF_Amplifiers pin for enable
  setupFIR();
  setupI2S();
  codec.set_voice_volume(70);
  Serial.println("I2S started...");


  WiFi.mode(WIFI_OFF);
  btStop();
  
  //changeFrequency( 14200000 );
  digitalWrite(AF_OUTPUT,HIGH); //Power amplifiers ON
}

void loop() 
{

//  copier.copy( *fir );
  copier.copy(*multi);

  // Update the Bounce instances :
  debouncer1.update();
  if ( debouncer1.changed() ) 
  {
    int deboucedInput = debouncer1.read();
    if ( deboucedInput == LOW ) {
      switch (directionState)
        {
  case 1:
    directionState = 2;
    break;
  case 2:
    directionState = 3;
    break;
  case 3:
    directionState = 4;
    break;
  case 4:
    directionState = 1;
    break;
  default:
    // statements
    break;
         }

      setupFIRchg();
      }
  }
  int inData = 0;
  if(Serial1.available() > 0)   // see if incoming serial data:
  {
    inData = Serial1.read();  // read oldest byte in serial buffer:
    if(inData == 'L')
     {
     //inData = 0;
     directionState = 1;
     setupFIRchg();
     }
    if(inData == 'U')
    {
    //inData = 0;
    directionState = 2;
    setupFIRchg();
    }
    if(inData == 'A')
     {
     //inData = 0;
     directionState = 3;
     setupFIRchg();
     }
    if(inData == 'F')
    {
    //inData = 0;
    directionState = 4;
    setupFIRchg();
    }

    if(inData == 'V')
    {
    int volume = Serial1.parseInt();
    volume = volume * 10;
    //inData = 0;
    codec.set_voice_volume( volume ); 
    }

  }

}



