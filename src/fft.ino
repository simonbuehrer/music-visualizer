/*
  fft.ino

  @author: Simon J. Buehrer
  @date: 16.03.2021
  
  Copyright (c) 2020 Simon J. Buehrer
  Permission is hereby granted, free of charge, to any person obtaining a copy
  of this software and associated documentation files (the "Software"), to deal
  in the Software without restriction, including without limitation the rights
  to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
  copies of the Software, and to permit persons to whom the Software is
  furnished to do so, subject to the following conditions:
  The above copyright notice and this permission notice shall be included in all
  copies or substantial portions of the Software.
  THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
  IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
  FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
  AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
  LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
  OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
  SOFTWARE.
 */
 
#include <arduinoFFT.h> //
#include <Adafruit_NeoPixel.h>

#define SAMPLES 128  // The FFT resolution (must be a power of 2)
#define LED_PIN 4    // LED pin  to communicate with the LED
#define LED_COUNT 9  // number of LED's used (= Total amount/3 for WS2811)



double vReal[SAMPLES];
double vImag[SAMPLES];


//Moving average stuff
#define numadress  5  // number of moving averages to calculate
#define numReadings  15  // moving average length
int readings [numadress][numReadings];
int readIndex[numadress];
long total[numadress];

//Define Objects
Adafruit_NeoPixel strip(LED_COUNT, LED_PIN, NEO_GRB + NEO_KHZ800);
arduinoFFT FFT = arduinoFFT();  // FFT object


void setup() {
  strip.begin();
  strip.show(); // Initialize all pixels to 'off'

  Serial.begin(9600);
  ADCSRA = 0b11100101;  // set ADC to free running mode and set pre-scalar to 32 (0xe5)
  ADMUX = 0b00000000;   // use pin A0 and external voltage reference
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


  int average_energy[5];
  int varity[5];
  for (int i = 0; i < 5; i++) {
    average_energy[i] = smooth(energy[i], i);
    varity[i] = energy[i] - average_energy[i];

    average_energy[i] = constrain(average_energy[i] -150, 0, 1800);
    average_energy[i] = map(average_energy[i], 0, 1800, 0, 1023);
    //Serial.print(average_energy[i]);
    //Serial.print(",");

    varity[i] = constrain(varity[i] - 20, 0, 2000);
    varity[i] = map(varity[i], 0, 2000, 0, 1023);
    Serial.print(peaks[i]);
    Serial.print(",");

    //strip.setPixelColor(i,  peaks[i], peaks[i], peaks[i]);
    //strip.show();

  }
  Serial.println(sizeof(int));


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
