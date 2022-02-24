#include <SPI.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

#define SCREEN_WIDTH 128 // OLED display width, in pixels
#define SCREEN_HEIGHT 64 // OLED display height, in pixels

// Declaration for an SSD1306 display connected to I2C (SDA, SCL pins)
#define OLED_RESET     -1 // Reset pin # (or -1 if sharing Arduino reset pin)
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

static const unsigned char PROGMEM wifi[] = {
  B00000001,B11111000,B00000000,
  B00001111,B11111111,B00000000,
  B00111110,B00001111,B11000000,
  B01111000,B00000001,B11100000,
  B11100001,B11111000,B01110000,
  B01000111,B11111110,B00100000,
  B00001111,B00001111,B00000000,
  B00011100,B00000011,B10000000,
  B00010000,B11110000,B10000000,
  B00000011,B11111100,B00000000,
  B00000111,B00001110,B00000000,
  B00000010,B00000100,B00000000,
  B00000000,B01100000,B00000000,
  B00000000,B11110000,B00000000,
  B00000000,B11110000,B00000000,
  B00000000,B01100000,B00000000
};

const unsigned char satellite[] PROGMEM = {
  B00000011,B11100000,
  B00000000,B00011000,
  B00000001,B11100100,
  B00000000,B00010010,
  B00000001,B11101010,
  B11000000,B00010100,
  B11100000,B00010000,
  B10111000,B10000000,
  B00011111,B10000000,
  B01011111,B10000000,
  B01011111,B11100000,
  B00111111,B11111000,
  B00011111,B11111100,
  B00001111,B11111110,
  B00000111,B11111000,
  B00011000,B00000000,
  B00001110,B00000000,
  B00101111,B00000000,
  B00111111,B00000000,
  B00111111,B10000000
};

const unsigned char bluetooth[] PROGMEM = {
  B00001111,B10000000,
  B00111111,B11100000,
  B01111111,B11110000,
  B01111101,B11110000,
  B11111100,B11111000,
  B11111100,B01111000,
  B11101101,B00111000,
  B11100101,B00111000,
  B11110000,B01111000,
  B11111000,B11111000,
  B11111000,B11111000,
  B11110000,B01111000,
  B11100101,B00111000,
  B11101101,B00111000,
  B11111101,B01111000,
  B11111100,B11111000,
  B01111101,B11110000,
  B01111101,B11110000,
  B00111111,B11100000,
  B00001111,B10000000
};

const unsigned char cellular[] PROGMEM = {
  B00100000,B00000100,
  B01100000,B00000110,
  B01001000,B00010010,
  B01011000,B00011010,
  B11010011,B11001011,
  B11010011,B11001011,
  B11010011,B11001011,
  B11010011,B11001011,
  B01011001,B10011010,
  B01001001,B10010010,
  B01100001,B10000110,
  B00100001,B10000100,
  B00000001,B10000000,
  B00000011,B11000000,
  B00000011,B11000000,
  B00000011,B11000000
};

const unsigned char battery[] PROGMEM = {
  B00000000,B00000100,B00000000,
  B00000000,B00001100,B00000000,
  B00000000,B00001100,B00000000,
  B00000000,B00011000,B00000000,
  B00111111,B11111111,B11000000,
  B11100000,B00010000,B01000000,
  B10100000,B00110000,B11000000,
  B10100000,B00100000,B11000000,
  B10100000,B01100001,B11000000,
  B10100000,B01000001,B11000000,
  B11100000,B11000011,B11000000,
  B00111111,B11111111,B11000000,
  B00000001,B10000000,B00000000,
  B00000001,B00000000,B00000000,
  B00000011,B00000000,B00000000,
  B00000010,B00000000,B00000000
};


void setup() {
  Serial.begin(115200);

  // SSD1306_SWITCHCAPVCC = generate display voltage from 3.3V internally
  if(!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) { 
    Serial.println(F("SSD1306 allocation failed"));
    for(;;); // Don't proceed, loop forever
  }
  // Show initial display buffer contents on the screen --
  // the library initializes this with an Adafruit splash screen.
  display.display();
  delay(2000); 
  display.clearDisplay();

  bool connections [] = {true, false, false};
  int counter = 0;
  
  for(int i=110;i>0;i--){
    oxygen_level(i, counter);
    communication_display(connections,2);
    check_battery(i);
    counter++;
    display.display();
    delay(400);
    display.clearDisplay();
  }
}

void loop() {
}

void check_battery(int life){
  if (life <= 20)
    display.drawBitmap(110,0,battery, 18, 16, 1);
}

void communication_display(bool out [], int in){
  int out_loc [] = {108,20};
  int in_loc [] = {107, 44};
  
  if (out[0] == true)       // connected to wifi
    display.drawBitmap(out_loc[0],out_loc[1], wifi, 20, 16, 1);
  else if (out[1] == true)  // connected to satellite
    display.drawBitmap(out_loc[0],out_loc[1], satellite, 16, 20, 1);
  else if (out[2] == true)  // connected to cellular
    display.drawBitmap(out_loc[0],out_loc[1], cellular, 16, 16, 1);

  // displays each connected bluetooth
  for (int i=0;i<in;i++){
    display.drawBitmap(in_loc[0] - i*20, in_loc[1], bluetooth, 13, 20, 1);
    display.setTextSize(1);
    display.setTextColor(WHITE);
    display.setCursor(in_loc[0] - i*20 + 14,in_loc[1] + 12); 
    display.println(i+1);
  }
}

void oxygen_level(int value, int counter) {
  display.setTextSize(3);
  display.setTextColor(WHITE);
  display.setCursor(0,20);
  display.println(value); 

  display.setTextSize(2);
  display.setCursor(0,0);

  // runs through possible states
  if(value <= 100 && value > 85){
    display.println("Stable");
  }
  else if(value <= 85 && value > 70){
    if (counter%4 == 0 || (counter - 1)%4 == 0)
      display.println("Caution");
  }
  else if(value <= 70 && value >= 0){
    if (counter%2 == 0)
      display.println("Warning");
  }
  else
    display.println("Error");
}
