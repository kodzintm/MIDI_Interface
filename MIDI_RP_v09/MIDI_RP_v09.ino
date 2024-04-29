#include <Adafruit_NeoPixel.h>
#include <Adafruit_TinyUSB.h>
#include <MIDI.h>
#include "Wire.h"
#include "TCA9555.h"
#include "ADS1X15.h"

Adafruit_USBD_MIDI usb_midi;
MIDI_CREATE_INSTANCE(Adafruit_USBD_MIDI, usb_midi, MIDI);
#define MIDI_LED 28

TCA9555 TCA(0x27, &Wire1);
ADS1115 ADS0(0x48, &Wire1);
ADS1115 ADS1(0x49, &Wire1);
ADS1115 ADS2(0x4A, &Wire1);
ADS1115 ADS3(0x4B, &Wire1);

//LEDy
#define LEDp 25
#define LEDn 24           //ilosc LEDow
int LEDk = 4;             //kanał MIDI dla LEDint
int LEDc[8][3] = {{0,0,0},{1,0,0},{0,1,0},{0,0,1},{1,1,0},{1,0,1},{0,1,1},{1,1,1}}; //7 kolorów LEDow RGB //w GO ustawiamy od 11 do 97, dziesiątki to moc a jednostki to kolor LED
int LEDm = 0;
int LEDcs[LEDn];
int LEDcl[LEDn];
Adafruit_NeoPixel pixels(LEDn, LEDp, NEO_GRB + NEO_KHZ800);

////kanał przyporządkowany do kolumny
int channel[24] {
  {1},{1},{1},{1},{2},{2},{2},{2},{3},{3},{0},{0},{0},{0},{0},{0},{0},{0},{0},{0},{0},{0},{0},{0}
};                        

////klawisz MIDI przyporzadkowany do pinu TCA względem każdej kolumny
int key[24][16]{
  {{36},{37},{38},{39},{40},{41},{42},{43},{44},{45},{46},{47},{48},{49},{50},{51}},  ///kolumna 1  /kanał 1
  {{52},{53},{54},{55},{56},{57},{58},{59},{60},{61},{62},{63},{64},{65},{66},{67}},  ///kolumna 2  /kanał 1
  {{68},{69},{70},{71},{72},{73},{74},{75},{76},{77},{78},{79},{80},{81},{82},{83}},  ///kolumna 3  /kanał 1
  {{84},{85},{86},{87},{88},{89},{90},{91},{92},{93},{94},{95},{96},{0},{0},{0}},     ///kolumna 4  /kanał 1

  {{36},{37},{38},{39},{40},{41},{42},{43},{44},{45},{46},{47},{48},{49},{50},{51}},  ///kolumna 5  /kanał 2
  {{52},{53},{54},{55},{56},{57},{58},{59},{60},{61},{62},{63},{64},{65},{66},{67}},  ///kolumna 6  /kanał 2
  {{68},{69},{70},{71},{72},{73},{74},{75},{76},{77},{78},{79},{80},{81},{82},{83}},  ///kolumna 7  /kanał 2
  {{84},{85},{86},{87},{88},{89},{90},{91},{92},{93},{94},{95},{96},{},{},{}},        ///kolumna 8  /kanał 2

  {{24},{25},{26},{27},{28},{29},{30},{31},{32},{33},{34},{35},{36},{37},{38},{39}},  ///kolumna 9  /kanał 3
  {{40},{41},{42},{43},{44},{45},{46},{47},{48},{49},{50},{51},{52},{53},{},{}},      ///kolumna 10 /kanał 3
  {{},{},{},{},{},{},{},{},{},{},{},{},{},{},{},{}},                                  ///kolumna 11 /kanał 
  {{},{},{},{},{},{},{},{},{},{},{},{},{},{},{},{}},                                  ///kolumna 12 /kanał 
  {{},{},{},{},{},{},{},{},{},{},{},{},{},{},{},{}},                                  ///kolumna 13 /kanał 
  {{},{},{},{},{},{},{},{},{},{},{},{},{},{},{},{}},                                  ///kolumna 14 /kanał 
  {{},{},{},{},{},{},{},{},{},{},{},{},{},{},{},{}},                                  ///kolumna 15 /kanał 
  {{},{},{},{},{},{},{},{},{},{},{},{},{},{},{},{}},                                  ///kolumna 16 /kanał 
  {{},{},{},{},{},{},{},{},{},{},{},{},{},{},{},{}},                                  ///kolumna 17 /kanał 
  {{},{},{},{},{},{},{},{},{},{},{},{},{},{},{},{}},                                  ///kolumna 18 /kanał 
  {{},{},{},{},{},{},{},{},{},{},{},{},{},{},{},{}},                                  ///kolumna 19 /kanał 
  {{},{},{},{},{},{},{},{},{},{},{},{},{},{},{},{}},                                  ///kolumna 20 /kanał 
  {{},{},{},{},{},{},{},{},{},{},{},{},{},{},{},{}},                                  ///kolumna 21 /kanał 
  {{},{},{},{},{},{},{},{},{},{},{},{},{},{},{},{}},                                  ///kolumna 22 /kanał 
  {{},{},{},{},{},{},{},{},{},{},{},{},{},{},{},{}},                                  ///kolumna 23 /kanał 
  {{},{},{},{},{},{},{},{},{},{},{},{},{},{},{},{}},                                  ///kolumna 24 /kanał 
};

int last_note[24][16] = {};
unsigned long Debounce = 50;    //debaunce dla klawiszy
unsigned long lastDebounce[24][16] = {};

int aON[4] = { 0,0,0,0 };    //włączenie skanowania analogów
int aV[4][4], aL[4][4];
int aK = 12;                //kanal MIDI dla analogow
int aMAP[4][4] = {{{0},{1},{2},{3}},{{4},{5},{6},{7}},{{8},{9},{10},{11}},{{12},{13},{14},{15}}};  //klawisz MIDI dla analogów
unsigned long tA = 1000;    //co ile czasu czytac analogi
unsigned long tA_last;

void setup() {
  TinyUSBDevice.clearConfiguration();
  TinyUSBDevice.setManufacturerDescriptor("MIDI Controler - elektronek.pl");
  TinyUSBDevice.setProductDescriptor("MIDI Controler - elektronek.pl");
  for (int pin = 0; pin < 24; pin++)
  {
    pinMode(pin, OUTPUT);
    digitalWrite(pin, LOW);
  }
  pinMode(MIDI_LED, OUTPUT);
  digitalWrite(MIDI_LED, HIGH);

  Wire1.setSDA(26);
  Wire1.setSCL(27);
  Wire1.setClock(1000000);
  Wire1.begin();  

  ADS0.begin();
  ADS0.setGain(0);      //  6.144 volt
  ADS0.setDataRate(7);  //  0 = slow   4 = medium   7 = fast
  ADS0.setMode(1);      //  continuous mode
  ADS1.begin();
  ADS1.setGain(0);      //  6.144 volt
  ADS1.setDataRate(7);  //  0 = slow   4 = medium   7 = fast
  ADS1.setMode(1);      //  continuous mode
  ADS2.begin();
  ADS2.setGain(0);      //  6.144 volt
  ADS2.setDataRate(7);  //  0 = slow   4 = medium   7 = fast
  ADS2.setMode(1);      //  continuous mode
  ADS3.begin();
  ADS3.setGain(0);      //  6.144 volt
  ADS3.setDataRate(7);  //  0 = slow   4 = medium   7 = fast
  ADS3.setMode(1);      //  continuous mode
  
  TCA.begin();
  TCA.pinMode16(0xFFFF);

  //MIDI.begin(MIDI_CHANNEL_OFF);
  //MIDI.begin(MIDI_CHANNEL_OMNI);
  MIDI.begin(LEDk);
  MIDI.turnThruOff();
  MIDI.setHandleNoteOn(LED_NoteOn);
  MIDI.setHandleNoteOff(LED_NoteOff);

  pixels.begin();
  led_test();

  delay(1000);
  digitalWrite(MIDI_LED, LOW);
}

void loop() {
  MIDI.read();
  readMatrix();	
  readAnalog();
  if ((millis() - tA_last) > tA) {
    tA_last = millis();
    lightLED();
  }
  //delay(2000);
}

void LED_NoteOn(byte channel, byte pitch, byte velocity) {
  LEDcs[pitch] = velocity;
}
void LED_NoteOff(byte channel, byte pitch, byte velocity) {
  LEDcs[pitch] = velocity;
}

void lightLED() {	
  for (int l = 0; l < LEDn; l++) {
    if(LEDcs[l] != LEDcl[l]){
      LEDm = LEDcs[l]/10;
      LEDm = map(LEDm, 1, 9, 1, 255);
      pixels.setPixelColor(l, pixels.Color(LEDc[LEDcs[l]-(LEDcs[l]/10)*10][0]*LEDm, LEDc[LEDcs[l]-(LEDcs[l]/10)*10][1]*LEDm, LEDc[LEDcs[l]-(LEDcs[l]/10)*10][2]*LEDm));
      pixels.show();
      LEDcl[l] = LEDcs[l];
    }
  }
  pixels.show();
}

void readMatrix() {
  for (int kol = 0; kol < 24; kol++) {
    digitalWrite(kol, HIGH);
    for (int pin = 0; pin < 16; pin++) {
      if ((millis() - lastDebounce[kol][pin]) > Debounce) {
        lastDebounce[kol][pin] = millis();
        if (TCA.read1(pin)){
          if(!last_note[kol][pin]){
            last_note[kol][pin] = true;
            MIDI.sendNoteOn(key[kol][pin], 127, channel[kol]);
          }
        }else{     
          if(last_note[kol][pin]){
            last_note[kol][pin] = false;
            MIDI.sendNoteOff(key[kol][pin], 0, channel[kol]);
          }   
        }
      }
    }
    digitalWrite(kol, LOW);
  }  
}

void readAnalog() {
  for (int pin = 0; pin < 4; pin++)
  {  
    if(aON[0]){
      ADS0.readADC(pin);
      aV[0][pin] = map(ADS0.getValue(),0,17570,0,127);
    }
    if(aON[1]){
      ADS1.readADC(pin);
      aV[1][pin] = map(ADS1.getValue(),0,17570,0,127);
    }
    if(aON[2]){
      ADS2.readADC(pin);
      aV[2][pin] = map(ADS2.getValue(),0,17570,0,127);
    }
    if(aON[3]){
      ADS3.readADC(pin);
      aV[3][pin] = map(ADS3.getValue(),0,17570,0,127);
    }
  }
  for (int ads = 0; ads < 4; ads++)
  {
    for (int pin = 0; pin < 4; pin++)
    {
      if (aV[ads][pin]!=aL[ads][pin]){
        MIDI.sendNoteOn(aMAP[ads][pin], aV[ads][pin], aK);
        aL[ads][pin] = aV[ads][pin];
      }
    }    
  }
}

void led_test() {  
  for (int i=0; i<LEDn; i++)
  {
    pixels.setPixelColor(i, pixels.Color(255,0,0));
    pixels.show();
    delay(10);
    pixels.setPixelColor(i, pixels.Color(0,0,0));
    
  }
  for (int i=0; i<LEDn; i++)
  {
    pixels.setPixelColor(i, pixels.Color(0,255,0));
    pixels.show();
    delay(10);
    pixels.setPixelColor(i, pixels.Color(0,0,0));
    
  }
  for (int i=0; i<LEDn; i++)
  {
    pixels.setPixelColor(i, pixels.Color(0,0,255));
    pixels.show();
    delay(10);
    pixels.setPixelColor(i, pixels.Color(0,0,0));
    
  }
  pixels.show();
  pixels.clear();
}