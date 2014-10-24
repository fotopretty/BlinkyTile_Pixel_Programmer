// BlinkyTile Pixel programmer
// Takes some commands from the serial and allows you to program WS282x LED modules
// Each pixel gets a unique color so it is easy to tell when it is programmed correctly.
//
// Should work with any 168, 328, or 32U4 based Arduino.
//
// Copyright 2014 Marty McGuire
// Simplified version of WS282x_Programmer Copyright 2014 Matt Mets

#include "DmxSimpleMod.h"

#define DATA_PIN    13      // Pin connected to the data output
#define ADDRESS_PIN  7      // Pin connected to the address programmer output

#define COMMAND_PROGRAM         'p'    // Program a channel

int maxPixel       = 14;    // Maximum pixel to display
int controllerType = 1;     // Controller type 1:WS2822S

uint8_t id_colors[14][3] = {
  {0,0,255},     // p1: deep blue
  {0,255,0},     // p2: green
  {0,255,255},   // p3: cyan
  {255,0,0},     // p4: red
  {255,0,255},   // p5: purple
  {255,255,0},   // p6: yellow
  {255,255,255}, // p7: white
  {0,0,127},     // p8: dim blue
  {0,127,0},     // p9: dim green
  {0,127,127},   // p10: dim cyan
  {127,0,0},     // p11: dim red
  {127,0,127},   // p12: dim purple
  {127,127,0},   // p13: dim yellow
  {127,127,127}, // p14: grey
};

void setup() {
  pinMode(DATA_PIN, OUTPUT);
  digitalWrite(DATA_PIN, LOW);
  
  pinMode(ADDRESS_PIN, OUTPUT);  
  digitalWrite(ADDRESS_PIN, HIGH);

  DmxSimple.usePin(DATA_PIN);
  DmxSimple.maxChannel(maxPixel*3);
  DmxSimple.begin();
  
  Serial.begin(19200);
}


void writePixel(int pixel, int r, int g, int b) {
  DmxSimple.write((pixel - 1)*3 + 1, b);
  DmxSimple.write((pixel - 1)*3 + 2, g);
  DmxSimple.write((pixel - 1)*3 + 3, r);   
}


uint8_t flipEndianness(uint8_t input) {
  uint8_t output = 0;
  for(uint8_t bit = 0; bit < 8; bit++) {
    if(input & (1 << bit)) {
      output |= (1 << (7 - bit));
    }
  }
  
  return output;
}

void programAddress(int address) {
  // Build the output pattern to program this address
  uint8_t pattern[3];
  
  if (controllerType ==  1) {
    // WS2822S (from datasheet)
    int channel = (address-1)*3+1;
    pattern[0] = flipEndianness(channel%256);
    pattern[1] = flipEndianness(240 - (channel/256)*15);
    pattern[2] = flipEndianness(0xD2);
  }
  else {
    // WS2821 (determined experimentally)
    int channel = (address)*3;
    pattern[0] = flipEndianness(channel%256);
    pattern[1] = flipEndianness(240 - (channel/256)*15);
    pattern[2] = flipEndianness(0xAE);    
  }

  DmxSimple.end();                 // Turn off the DMX engine
  delay(50);                       // Wait for the engine to actually stop
  digitalWrite(ADDRESS_PIN, HIGH); // Set the address output pin high if it wasn't already
  digitalWrite(DATA_PIN, LOW);     // Force the data pin low
  delay(50);                       // Let this sit for a bit to signal the chip we are starting
  
  DmxSimple.usePin(ADDRESS_PIN);   // Use the address output pin to generate the DMX signal
  DmxSimple.maxChannel(3);
  DmxSimple.write(1, pattern[0]);  // Pre-load the output pattern (note this might trash the LED output, meh)
  DmxSimple.write(2, pattern[1]);
  DmxSimple.write(3, pattern[2]);

  digitalWrite(ADDRESS_PIN, LOW);   // Set the address pin low to begin transmission
  delay(1000);                      // Delay 1s to signal address transmission begin
  DmxSimple.begin();                // Begin transmission. Only the first one actually counts.

  delay(20);                        // Wait a while for the the signal to be sent
  DmxSimple.end();
  digitalWrite(ADDRESS_PIN, HIGH);  // Set the address output pin high if it wasn't already

  // Reset the output, and set the output to our new pixel address.
  DmxSimple.usePin(DATA_PIN);
  DmxSimple.maxChannel(maxPixel*3);
  DmxSimple.begin();
}


void color_loop() {  
  float mult = 0.1;
  for (uint8_t i = 0; i < maxPixel; i+=1) {
    writePixel(i+1,
      mult * id_colors[i][0],
      mult * id_colors[i][1],
      mult * id_colors[i][2]
    );
  }
}


void loop() {
  
  if(Serial.available() > 0) {
    char command = Serial.read();
    int parameter = Serial.parseInt();
    
    if(command == COMMAND_PROGRAM) {
      if(parameter < 1 || parameter > maxPixel) {
        parameter = 1;
      }
      
      Serial.print("Programming pixel to address: ");
      Serial.println(parameter);
      programAddress(parameter);
    }
  }

  color_loop();
}
