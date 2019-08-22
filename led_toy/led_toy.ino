#include <Adafruit_NeoPixel.h>

static uint16_t hue;

// Which pin on the Arduino is connected to the NeoPixels?
#define PIN 4

// How many NeoPixels are attached to the Arduino?
#define NUMPIXELS 12

// Time to turn leds off when wheel is not rotating
#define TIME_TO_IDLE 5000

#define MAX_CURRENT 500 // total current limit in mA, one ws2812 LED needs 20 mA per color
#define ONE_LED_CURRENT 20 // mA
// some colors like yellow needs to be red and green LEDs active at same time, so total current for this color will be 40 mA
#define BRIGHTNESS  (((MAX_CURRENT / NUMPIXELS) * 255) / (ONE_LED_CURRENT * 2))

#if BRIGHTNESS>255
 #undef BRIGHTNESS
 #define BRIGHTNESS 255
#endif

#define ENCODER_A 2 // external interrupt pins of ATMega38p
#define ENCODER_B 3
#define GET_ENCODER_STATE ((PIND & 0x0C) >> 2) // PIND2 and PIND3 shifted to be bit0 and bit1

Adafruit_NeoPixel pixels(NUMPIXELS, PIN, NEO_GRB + NEO_KHZ800);

volatile int32_t  encoderPosition = 0; // variable must be defined as volatile to use in the interrupt
volatile uint8_t  encCurrState, encPrevState = 0;
volatile int8_t   encDir = 0;

const uint8_t encState[4] = { 0, 1, 3, 2 };

const int8_t encDirTable[4][4] = { // first dimension is current state
  { 0,  0, -1,  0 },               // second dimension is previous state
  { 0,  0,  0,  1 },               // result is rotation direction from previous state
  { 1,  0,  0, -1 },               // state is a bitwise OR result of A and B states 
  { 0, -1,  1,  0 }                // shifted to the bit 0 and bit 1 accordingly
};

void setup() {
  pixels.begin(); // INITIALIZE NeoPixel strip object (REQUIRED)

  pinMode(ENCODER_A, INPUT);
  pinMode(ENCODER_B, INPUT);

  digitalWrite(ENCODER_A, 1); // pulling UP encoder pins
  digitalWrite(ENCODER_B, 1); // and connect middle pin of encoder to GND

  attachInterrupt(digitalPinToInterrupt(ENCODER_A), EncoderISR, CHANGE); // interrupt will fire at any change of pin state
  attachInterrupt(digitalPinToInterrupt(ENCODER_B), EncoderISR, CHANGE);
                      
  encPrevState = GET_ENCODER_STATE; // define initial encoder state
}

void EncoderISR() { // do not use delay() or Serial.print() here
  encCurrState = GET_ENCODER_STATE;

  encDir = encDirTable[encCurrState][encPrevState];

  encoderPosition += encDir;

  encPrevState = encCurrState;
}

void loop() {
  static int32_t pos = 0;
  static uint32_t lastMove = 0;
  uint16_t tmpHue;
  
  if (pos != encoderPosition) {
    lastMove = millis();

    // HUE value 0..65535 or full range of uint16_t
    hue += (encoderPosition - pos) * 5000; // one encoder click to 5000 hue value
    
    tmpHue = hue;

    pos = encoderPosition;

    for(int i = 0; i < NUMPIXELS; i++) {
      //                                      HUE, saturation (0..255) and value (brightness, 0..255) 
      pixels.setPixelColor(i, pixels.ColorHSV(tmpHue, 255, BRIGHTNESS));
      tmpHue += (65535 / NUMPIXELS); // 65535 / leds count to show full rainbow
    }
    
    pixels.show();
  }

  if (millis() - lastMove > TIME_TO_IDLE) {
    pixels.clear(); // Set all pixel colors to 'off'

    pixels.show();
  }
}
