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
  bool sensors [] = {true, true};
  int counter = 0;
  
  for(int i=110;i>0;i--){
    oxygen_level(i, i, sensors, counter);
    communication_display(connections);
    check_battery(i);
    counter++;
    if (i <= 80 && i > 50){
      connections[0] = false;
      connections[1] = true;
      sensors[0] = false;
    }
    else if (i <= 50){
      connections[1] = false;
      connections[2] = true;
      sensors[1] = false;
    }
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

void communication_display(bool out []){
  int out_loc [] = {108,20};
  
  if (out[0] == true)       // connected to wifi
    display.drawBitmap(out_loc[0],out_loc[1], wifi, 20, 16, 1);
  else if (out[1] == true)  // connected to satellite
    display.drawBitmap(out_loc[0],out_loc[1], satellite, 16, 20, 1);
  else if (out[2] == true)  // connected to cellular
    display.drawBitmap(out_loc[0],out_loc[1], cellular, 16, 16, 1);
}

void oxygen_level(int c_value, int h_value, bool sensors [], int counter){
  display.setTextColor(WHITE);
  int bl_loc [] = {107, 44};

  // if bluetooth 1 is connection print info
  if (sensors[0]){
    display.drawBitmap(115, 44, bluetooth, 13, 20, 1);
    display.setCursor(99, 56);
    display.setTextSize(1); 
    display.println("c");
    display.setCursor(0,50);
    display.println("calf");
    display.setTextSize(3.5);
    display.setCursor(0,20);
    display.println(c_value);
  }
  else{
    display.setTextSize(1); 
    display.setCursor(0,50);
    display.println("calf");
    c_value = 100;
  }

  // if bluetooth 2 is connection print info
  if (sensors[1]){
    display.drawBitmap(115, 44, bluetooth, 13, 20, 1);
    display.setCursor(107, 56);
    display.setTextSize(1); 
    display.println("h");
    display.setCursor(60,50);
    display.println("hand");
    display.setTextSize(3.5);
    display.setCursor(60,20);
    display.println(h_value);
  }
  else{
    display.setTextSize(1); 
    display.setCursor(50,50);
    display.println("hand");
    h_value = 100;
  }
    
  display.setTextSize(2);
  display.setCursor(0,0);

  // runs through possible states
  if (!sensors[0] && !sensors[1])
    display.println("Connect");
  else if((c_value <= 100 && c_value > 85) && (h_value <= 100 && h_value > 85)){
    display.println("Stable");
  }
  else if((c_value <= 85 && c_value > 70) || (h_value <= 85 && h_value > 70)){
    if (counter%4 == 0 || (counter - 1)%4 == 0)
      display.println("Caution");
  }
  else if((c_value <= 70 && c_value >= 0) || (h_value <= 70 && h_value >= 0)){
    if (counter%2 == 0)
      display.println("Warning");
  }
  else
    display.println("Error");
}
