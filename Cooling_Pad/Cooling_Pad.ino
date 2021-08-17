#include <FastLED.h>
#include <math.h>

/////////////////////CONFIGURACIÓN LEDS//////////////////////
//Variables de los LEDs
#define LED_PIN_1   10     //Pin Tira LED 1
#define LED_PIN_2   11     //Pin Tira LED 2
#define NUM_LEDS    3      //Número de LEDs en cada tira (si es ws2811 numLeds/3)
#define BRIGHTNESS  220    //Brillo máximo 0-255
#define LED_TYPE    WS2811 //Tipo de tira LED (normalmente Ws2812 o WS2812)
///////////////////////////////////////////////////////////

#define COLOR_ORDER GRB
#define UPDATES_PER_SECOND 100
CRGB leds1[NUM_LEDS];
CRGB leds2[NUM_LEDS];
CRGBPalette16 currentPalette = RainbowColors_p;
TBlendType    currentBlending = LINEARBLEND;
extern CRGBPalette16 myRedWhiteBluePalette;
extern const TProgmemPalette16 myRedWhiteBluePalette_p PROGMEM;

/////////////////////CONFIGURACIÓN TERMISTOR////////////////////////
//Variables Termistor
const int Rc = 10000;   //Valor resistencia divisor de tensión
const int Vcc = 5;
const int termistorPin = A1;
float A = 1.11492089e-3;     
float B = 2.372075385e-4;  
float C = 6.954079529e-8;
float K = 2.5; //factor de disipacion en mW/C
/////////////////////////////////////////////////////////////////////

/////////////////////CONFIGURACIÓN VENTILADORES//////////////////////
//Variables Ventiladores
const int fanPin = 3;     //Pin con PWM (tiene que ser 3 en este caso)
const int fanSwitch = 6;  //Pin para encendido y apagado total de los ventiladores
/////////////////////////////////////////////////////////////////////

/////////////////////CONFIGURACIÓN BOTONES//////////////////////
//Variables Botones de Control
const int ledsButton = 2;    //Pin Botón de los LEDs
const int fanButton = 5;     //Pin Botón de los modos de ventilador
bool antLedsButtonState = false;
bool antFanButtonState = false;
////////////////////////////////////////////////////////////////

bool fanOn = true;
String fanMode = "maxpower"; //Modos: maxpower, automatic y off (se pueden ampliar)
String ledsMode = "on"; //Modos: on y off (se pueden ampliar)


//Prototipos de las funciones
float measureTemperature();
void configure25KHzPwm();
void fanSpeedHandler(float temperature);


void setup() {
  // put your setup code here, to run once:
  pinMode(fanPin, OUTPUT);  //Pin PWM de ventiladores
  pinMode(fanSwitch, OUTPUT); //Pin transistor (interruptor)

  //Configuración de los botones
  pinMode(ledsButton, INPUT);
  pinMode(fanButton, INPUT);  

  //Configuración LEDs
  FastLED.addLeds<LED_TYPE, LED_PIN_1, COLOR_ORDER>(leds1, NUM_LEDS);
  FastLED.addLeds<LED_TYPE, LED_PIN_2, COLOR_ORDER>(leds2, NUM_LEDS);
  FastLED.setBrightness(  BRIGHTNESS );
  
  configure25KHzPwm();
}

void loop() {
  // put your main code here, to run repeatedly:
  ////////////FANS CONTROL/////////////
  bool fanButtonState = digitalRead(fanButton);
  if(fanButtonState == true && antFanButtonState == false){
    if(fanMode == "automatic") fanMode = "off";
    else if (fanMode == "off") fanMode = "maxpower";
    else fanMode = "automatic";
  }
  antFanButtonState = fanButtonState;

  if(fanMode == "maxpower"){
    //Ventilador al 100% de su capacidad
    digitalWrite(fanSwitch, HIGH);
    fanSpeedHandler(100);  
  }

  else if(fanMode == "automatic"){
    float temperature = measureTemperature();
    if(temperature >= 40){
      digitalWrite(fanSwitch, HIGH);
      fanSpeedHandler(temperature);
    }
    else{
      digitalWrite(fanSwitch, LOW);
    }
  }

  else if(fanMode == "off"){
    digitalWrite(fanSwitch, LOW);
  }

  ///////////LEDS CONTROL///////////
  bool ledsButtonState = digitalRead(ledsButton);
  if(ledsButtonState == true && antLedsButtonState == false){
    if(ledsMode == "on") ledsMode = "off";
    else ledsMode = "on";
  }
  antLedsButtonState = ledsButtonState;

  if(ledsMode == "off"){
    FastLED.setBrightness(0);
    FastLED.show();
  }

  else if(ledsMode == "on"){
    FastLED.setBrightness(BRIGHTNESS);
    ChangePalettePeriodically();
    static uint8_t startIndex = 0;
    startIndex = startIndex + 1; /* motion speed */
    FillLEDsFromPaletteColors( startIndex);
    FastLED.show();
  }
  
  FastLED.delay(1000 / UPDATES_PER_SECOND);
}

float measureTemperature(){
  float raw = analogRead(termistorPin);
  float V = raw / 1024*Vcc;

  float R = (Rc*V) / (Vcc-V);

  float logR  = log(R);
  float R_th = 1.0 / (A + B * logR + C * logR * logR * logR );

  float kelvin = R_th - V*V/(K * R)*1000;
  float celsius = kelvin - 273.15;

  return celsius;
}

void configure25KHzPwm(){
  TCCR2A = 0x23;
  TCCR2B = 0x0A;
  OCR2A = 79;
  OCR2B = 0;
}


void fanSpeedHandler(float temperature){ 
  if(temperature > 70){
    temperature = 70;
  }
  //40ºC velocidad al mínimo
  //70ºC o más, velocidad al máximo
  OCR2B = int(map(temperature, 40, 70, 0, 79));
}


//Funciones ColorPalette
void FillLEDsFromPaletteColors( uint8_t colorIndex)
{
    uint8_t brightness = 255;
    
    for( int i = 0; i < NUM_LEDS; ++i) {
        leds1[i] = ColorFromPalette( currentPalette, colorIndex, brightness, currentBlending);
        leds2[i] = leds1[i];
        colorIndex += 3;
    }
}


// There are several different palettes of colors demonstrated here.
//
// FastLED provides several 'preset' palettes: RainbowColors_p, RainbowStripeColors_p,
// OceanColors_p, CloudColors_p, LavaColors_p, ForestColors_p, and PartyColors_p.
//
// Additionally, you can manually define your own color palettes, or you can write
// code that creates color palettes on the fly.  All are shown here.

void ChangePalettePeriodically()
{
    uint8_t secondHand = (millis() / 1000) % 90;
    static uint8_t lastSecond = 99;
    
    if( lastSecond != secondHand) {
        lastSecond = secondHand;
        
        if( secondHand ==  0)  { currentPalette = RainbowColors_p;}
        if( secondHand == 10)  { currentPalette = RainbowStripeColors_p;}
        if( secondHand == 20)  { SetupPurpleAndGreenPalette();}
        if( secondHand == 30)  { currentPalette = RainbowColors_p;}
        if( secondHand == 40)  { currentPalette = LavaColors_p; }
        if( secondHand == 50)  { currentPalette = LavaColors_p;}
        if( secondHand == 60)  { currentPalette = OceanColors_p;}
        if( secondHand == 70)  { currentPalette = CloudColors_p;}
        if( secondHand == 80)  { currentPalette = PartyColors_p;}
        if( secondHand == 90)  { currentPalette = myRedWhiteBluePalette_p;}
    }
}

// This function fills the palette with totally random colors.
void SetupTotallyRandomPalette()
{
    for( int i = 0; i < 16; ++i) {
        currentPalette[i] = CHSV( random8(), 255, random8());
    }
}

// This function sets up a palette of black and white stripes,
// using code.  Since the palette is effectively an array of
// sixteen CRGB colors, the various fill_* functions can be used
// to set them up.
void SetupBlackAndWhiteStripedPalette()
{
    // 'black out' all 16 palette entries...
    fill_solid( currentPalette, 16, CRGB::Black);
    // and set every fourth one to white.
    currentPalette[0] = CRGB::White;
    currentPalette[4] = CRGB::White;
    currentPalette[8] = CRGB::White;
    currentPalette[12] = CRGB::White;
    
}

// This function sets up a palette of purple and green stripes.
void SetupPurpleAndGreenPalette()
{
    CRGB purple = CHSV( HUE_PURPLE, 255, 255);
    CRGB green  = CHSV( HUE_GREEN, 255, 255);
    CRGB black  = CRGB::Black;
    
    currentPalette = CRGBPalette16(
                                   green,  green,  black,  black,
                                   purple, purple, black,  black,
                                   green,  green,  black,  black,
                                   purple, purple, black,  black );
}


// This example shows how to set up a static color palette
// which is stored in PROGMEM (flash), which is almost always more
// plentiful than RAM.  A static PROGMEM palette like this
// takes up 64 bytes of flash.
const TProgmemPalette16 myRedWhiteBluePalette_p PROGMEM =
{
    CRGB::Red,
    CRGB::Gray, // 'white' is too bright compared to red and blue
    CRGB::Blue,
    CRGB::Black,
    
    CRGB::Red,
    CRGB::Gray,
    CRGB::Blue,
    CRGB::Black,
    
    CRGB::Red,
    CRGB::Red,
    CRGB::Gray,
    CRGB::Gray,
    CRGB::Blue,
    CRGB::Blue,
    CRGB::Black,
    CRGB::Black
};
