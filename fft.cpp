
#include <arduinoFFT.h>
#include <Adafruit_NeoPixel.h>
#include <SPI.h>

#define SAMPLES 128            //Must be a power of 2

#define LED_PIN 4
#define LED_COUNT 9


//fft var
double vReal[SAMPLES];
double vImag[SAMPLES];


//Moving average stuff
const int numadress  = 5;
const int numReadings  = 20;
int readings [numadress][numReadings];
int readIndex[numadress];
long total[numadress];



Adafruit_NeoPixel strip(LED_COUNT, LED_PIN, NEO_GRB + NEO_KHZ800);
arduinoFFT FFT = arduinoFFT();  // FFT object


void setup() {
  strip.begin();
  strip.show(); // Initialize all pixels to 'off'

  Serial.begin(9600);
  ADCSRA = 0b11100101;      // set ADC to free running mode and set pre-scalar to 32 (0xe5)
  ADMUX = 0b00000000;       // use pin A0 and external voltage reference
  delay(50);            // wait to get reference voltage stabilized
}

void loop() {
  // ++ Sampling
  for (int i = 0; i < SAMPLES; i++)
  {

    while (!(ADCSRA & 0x10));       // wait for ADC to complete current conversion ie ADIF bit set
    ADCSRA = 0b11110101 ;           // clear ADIF bit so that ADC can do next operation (0xf5)
    int value = ADC - 512 ;         // Read from ADC and subtract DC offset caused value
    vReal[i] = value;               // Copy to bins after compressing
    vImag[i] = 0;

  }


  //--------------------------------------------------------------
  // FFT Sampling
  //--------------------------------------------------------------
  FFT.Windowing(vReal, SAMPLES, FFT_WIN_TYP_HAMMING, FFT_FORWARD);
  FFT.Compute(vReal, vImag, SAMPLES, FFT_FORWARD);
  FFT.ComplexToMagnitude(vReal, vImag, SAMPLES);


  //--------------------------------------------------------------
  // re-arrange the results
  // | FFT | from freq. | to freq. |
  // |   0 |        0Hz |    150Hz |
  // |   1 |      151Hz |    750Hz |
  // |   2 |      751Hz |   1050Hz |
  // | ... |        ... |      ... |
  // |  63 |    18751Hz |  19050Hz |
  //--------------------------------------------------------------
  int energy[5];

  //Sub-Bass (0-150Hz)
  energy[0] = vReal[0];

  //Bass (151-450Hz)
  energy[1] = vReal[1];

  //Low-Mid (451-1950Hz)
  for (int i = 2; i <= 6; i++) {
    energy[2] += vReal[i];
  }
  energy[2] = energy[2] / 2;

  //Mid (1951-7350Hz)
  for (int i = 7; i <= 24; i++) {
    energy[3] += vReal[i];
  } energy[3] = energy[3] / 2;

  //High (7351-19050Hz)
  for (int i = 25; i <= 63; i++) {
    energy[4] += vReal[i];
  } energy[4] = energy[4] / 2;

  int peaks[6];
  int average_energy[5];
  for (int i = 0; i < 5; i++) {
    average_energy[i] = smooth(energy[i], i);
    average_energy[i] = energy[i] - average_energy[i];
    average_energy[i] = constrain(average_energy[i] - 20, 0, 5000);
    average_energy[i] = map(average_energy[i], 0, 5000, 0, 255);

    Serial.print(average_energy[i]);
    Serial.print(",");


    peaks[i] = peaks[i] - 8;
    peaks[i] = (peaks[i] <= average_energy[i]) ? average_energy[i] : peaks[i] ;
    strip.setPixelColor(i,  peaks[i], peaks[i], peaks[i]);
    strip.show();

  }
  Serial.println();


}

//--------------------------------------------------------------
// calculating mooving average and variance function
//--------------------------------------------------------------
long smooth(int ugly_value, int adress) {
  long average;
  total[adress] = total[adress] - readings[adress][readIndex[adress]];
  readings[adress][readIndex[adress]] = ugly_value;
  total[adress] = total[adress] + readings[adress][readIndex[adress]];
  readIndex[adress]++;
  if (readIndex[adress] >= numReadings) {
    readIndex[adress] = 0;
  }
  average = total[adress] / numReadings;
  return average;
}