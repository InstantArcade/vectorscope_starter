
/**
 * @file main.cpp
 * @author Bob Hickman (Instant Arcade - www.instantarcade.com)
 * @brief C++ demo for the Supercon Vectorscop Badge (also builds on Waveshare Circular display)
 * @version 0.1
 * @date 2023-12-21
 * 
 * @copyright Copyright (c) 2023 Bob Hickman (Instant Arcade)
 * 
 * MIT License, so go crazy with it
 */
#include <Arduino.h>
#include <TFT_eSPI.h>
#include "hardware/spi.h"
#include "pico/stdlib.h"
#include <cmath>
#include <vector>
#include <algorithm>


#include "pacfont.h"
// #include "amigafont.h"
#include "amiga_wood.h"

constexpr int WIDTH = 240;
constexpr int HEIGHT = 240;

TFT_eSPI tft = TFT_eSPI();

#define SWAP_BYTES(x) (((x<<8)&0xFF00) | (x>>8))
#define PACK_RGB_565(r,g,b) ((r>>3)<<11 | (g>>2)<<5 | (b>>3))
#define PACK_RGB_565_SWAPPED(r,g,b) SWAP_BYTES(((r>>3)<<11 | (g>>2)<<5 | (b>>3)))

// define some default colors
constexpr uint16_t WHITE  = PACK_RGB_565_SWAPPED(255,255,255);
constexpr uint16_t RED  = PACK_RGB_565_SWAPPED(255,0,0);
constexpr uint16_t GREEN  = PACK_RGB_565_SWAPPED(0,255,0);
constexpr uint16_t BLUE  = PACK_RGB_565_SWAPPED(0,0,255);
constexpr uint16_t YELLOW  = PACK_RGB_565_SWAPPED(255,255,0);
constexpr uint16_t MAGENTA  = PACK_RGB_565_SWAPPED(255,0,255);
constexpr uint16_t CYAN  = PACK_RGB_565_SWAPPED(0,255,255);

float sintab[360];
uint16_t screen_buffer[240*240]; // back buffer for the screen

// cosine is a 90 degree offset into the sin table
void generate_sintab()
{
    for( auto f=0; f < 360; f++ )
    {
        auto rad = f/180.0f * M_PI;
        sintab[f] = sinf(rad);
    }
}

void clear_buffer( uint16_t rgb565color = 0 )
{
    for(auto i=0; i <240*240; i++)
    {
        screen_buffer[i]=rgb565color;
    }
}

// Draw a message using the Amiga font (16x16 pixels) - overwrite the color by passing a non-zero color value
void write_string_amiga( int x, int y, const char *str, int color565, bool center = false )
{
    // Amiga font is 16x16 per character
    constexpr short AMIGA_FONT_SIZE = 16;

    // image is 512px wide (32 charcters wide) by 48px (3 rows) tall
    constexpr short AMIGA_FONT_WIDTH_BYTES = 64; // 32 x 16pixels = 32 * 2bytes = 64
    // Starts with chr #33 "!"
    constexpr short AMIGA_FONT_START_CHAR = 33; // Starting character

    int len = (int)strlen(str);
    if( center )
    {
        x -= (len*AMIGA_FONT_SIZE)/2; // center on passed in x positon
    }
    
    for( int idx = 0; idx < len; idx++ ) // go through the message
    {
        char c = str[idx]; // get a character
        
        if( c <= 32) continue; // unprintable or space, so skip it
        c -= AMIGA_FONT_START_CHAR; // adjust for our font start
        
        int row = c/(AMIGA_FONT_WIDTH_BYTES/2);
        int col = c%(AMIGA_FONT_WIDTH_BYTES/2);
        int src = row*image_width_amiga_wood*AMIGA_FONT_SIZE+col*AMIGA_FONT_SIZE;   // where I'm reading this char data from
        int dst = x + y*WIDTH;                        // where in the screen buffer I'm writing to
      
        for( int r = 0; r < AMIGA_FONT_SIZE; r++ ) // each char row (height of a single glyph in the font data)
        {
            for( int p = 0; p < AMIGA_FONT_SIZE; p++ ) // each horizontal pixel of the glyph
            {
                if( (dst > 0) && (dst < WIDTH*HEIGHT*2) ) // within the screen buffer?
                {
                    if( image_data_amiga_wood[src] != 0 ) // source pixel is black?
                    {
                        
                        if( color565 != 0 )
                        {
                            screen_buffer[dst] = color565; // override source pixel color
                        }
                        else
                        {
                            screen_buffer[dst] = image_data_amiga_wood[src]; // use origin source pixel color
                        }
                    }
                    src++;
                    dst++;
                }
            }
            
            dst += WIDTH-AMIGA_FONT_SIZE; // next line down on screen buffer
            src += image_width_amiga_wood-AMIGA_FONT_SIZE; //  next line down on source buffer
        } // next row
        x += AMIGA_FONT_SIZE; // move along one character width (in pixels)
    }
}


// Draw a message using the Pac-Man font (8x8 pixels, one bit) - sinc this in a mono font, you need to specify the color (rgb565 packed and byte swapped)
void write_pac_string( int x, int y, const char *str, int color565, bool center = false )
{
   int len = (int)strlen(str);
   if( center )
   {
       x = 119 - (len*8)/2;
   }
   for( int idx = 0; idx < len; idx++ ) // go through the message
   {
       char c = str[idx]; // get a character

       // is it valid?
       int validlen = strlen(pac_font_lookup); // how big is the lookup string?
       for( int v = 0; v < validlen; v++ ) // loop through it to find a match
       {
           if( pac_font_lookup[v] == c ) // this is a printable character
           {
               // print it
               int row = v/10;
               int col = v%10;
               int src = row*80+col;   // where I'm reading this char data from (1 byte is one char row)
               int dst = x + y*240;    // where in the screen buffer I'm writing to (16 bytes is one char row)

               for( int row = 0; row < 8; row++ ) // each char row
               {
                   for( int b = 0; b < 8; b++ ) // go through each bit (columns)
                   {
                       if( (dst > 0) && (dst < 240*240*2) )
                       {
                           if( pac_font[src] & (128 >> b) ) // check this byte bit by bit
                           {
                               // bit set?
                               screen_buffer[dst] = color565;
                           }
                           else
                           {
                               // write background color or nothing
                               // screen_buffer[dst] = 0x1FF8;
                           }
                       }
                       dst++;
                   }

                   dst += 240-8; // next line down on screen buffer
                   src += 10; //  next line down on source buffer
               }
              break; // finished drawing this char, so don't continue lookups
           } // next row
       }
       x += 8;
   }
}

void write_curved_pac_string( int x_center, int y_center, const char *str, int color565, float angle, float radius )
{
   float letter_spacing_deg = 10;
   int len = (int)strlen(str);

   float anglestart = angle - (len/2.0f)*(letter_spacing_deg)*1; // might need to involve radius here also
   while( anglestart < 0.0f ) anglestart += 360.0f;
   while( anglestart > 360.0f ) anglestart -= 360.0f;

   for( int idx = 0; idx < len; idx++ ) // go through the message
   {
       char c = str[idx]; // get a character

       // is it valid?
       int validlen = strlen(pac_font_lookup); // how big is the lookup string?
       for( int v = 0; v < validlen; v++ ) // loop through it to find a match
       {
           if( pac_font_lookup[v] == c ) // this is a printable character
           {
               int src_row = v/10;
               int src_col = v%10;
               int src = src_row*80+src_col;   // where I'm reading this char data from (1 byte is one char row)

               for( int crow = 0; crow < 8; crow++ ) // each char row
               {
                   float curangle = anglestart;//+crow*1.0f;
                   for( int b = 0; b < 8; b++ ) // go through each bit (columns)
                   {
                       uint16_t sti = 360-((int)((curangle+idx*letter_spacing_deg)-b*0.1f))%360;
                       uint16_t cti = (sti+90)%360;
                       if( pac_font[src] & (128 >> b) ) // check this byte bit by bit
                       {
                           // find drawing x/y
                           int tx = x_center + (int)(sintab[sti] * (radius-(crow*2)));
                           int ty = y_center + (int)(sintab[cti] * (radius-(crow*2)));
                           int dst = tx + ty*240;    // where in the screen buffer I'm writing to (16 bytes is one char row)

                           if( (dst > 0) && (dst < 240*240*2) ) // on screen?
                           {
                               screen_buffer[dst] = color565;
                           }
                       } // <- bit is set
                       curangle += 1.0f;//*multiplier; // might need to tweak this based on radius
                   } // <-  next bit
                   src += 10; // source pixels on next row
               } // <- next row
           } // <- char matched and is printable
       } // <- find match loop
   } // <- each char
}

void buffer_to_screen()
{
    tft.startWrite();
    tft.setAddrWindow(0,0,240,240);
    tft.pushPixelsDMA(screen_buffer,240*240);
    tft.endWrite();
}

enum BUTTONS{
    BUT_SAVE    = 0, // same bits but on different input lines
    JOY_RIGHT   = 0, // row 1

    BUT_MENU    = 1,// row 0
    JOY_UP      = 1,// row 1

    BUT_WAVE    = 2,// row 0
    JOY_BUT     = 2,// row 1

    BUT_SCOPE   = 3,// row 0
    JOY_LEFT    = 3,// row 1

    BUT_XY      = 4,// row 0
    JOY_DOWN    = 4,// row 1

    BUT_B       = 5,// row 0
    BUT_A       = 5,// row 1

    BUT_D       = 6,// row 0
    BUT_LEVEL   = 6,// row 1

    BUT_C       = 7,// row 0
    BUT_RANGE   = 7,// row 1

    BUT_INVALID = 99,
};

constexpr int PIN_LOAD = 16;
constexpr int PIN_SDI = 1;
constexpr int PIN_SCL = 0;
constexpr int PIN_ROW0 = 17;
constexpr int PIN_ROW1 = 18;

unsigned short bdata = 0b1100000000000001;


// LED bits
constexpr unsigned char BIT_X = 0;
constexpr unsigned char BIT_Y = 1;
constexpr unsigned char BIT_SINE = 2;
constexpr unsigned char BIT_SQUARE = 3;
constexpr unsigned char BIT_TRIANGLE = 4;
constexpr unsigned char BIT_SAW = 5;
constexpr unsigned char BIT_SCOPE = 6;
constexpr unsigned char BIT_SIGGEN = 7;

unsigned char leds = 0;
unsigned char butstate[2] = {0,0}; // inverted, from HW, so bit is set if button is down

void set_led( byte which )
{
    leds |= (1<<which);
}

bool get_led( byte which )
{
    return leds & (1<<which);
}

void read_but_state()
{
    butstate[0]=butstate[1]=0;
    #ifdef VECTORSCOPE_BADGE
        // clock in the LED states and the column I need for the buttons
        // two bytes,
        // 8 bits of LEDs, * Bits to set button read column

        // Load - GPIO 16
        // Clock - GPIO 0
        // Data IN - GPIO 1

        // Row 0 - GPIO 17 - active LOW
        // Row 1 - GPIO 18 - active LOW
        for( auto columns = 0; columns < 8; columns++ )
        {
            // latch the data
            digitalWrite(PIN_LOAD,1);
            // latch the data
            digitalWrite(PIN_LOAD,0);
            bool bit = false;

            // scan each key column
            uint16_t iodata = (leds << 8) | (1<<columns);

            for( auto i = 0; i < 16; i++ )
            {
                bit = iodata & (1 << i) ;
                digitalWrite(PIN_SDI, bit);
                // clock it
                digitalWrite(PIN_SCL,1);
                digitalWrite(PIN_SCL,0);
            }
            // latch the data
            digitalWrite(PIN_LOAD,1);
            // latch the data
            digitalWrite(PIN_LOAD,0);

            unsigned char b17, b18;
            b17 = digitalRead(PIN_ROW0);
            b18 = digitalRead(PIN_ROW1);
            if( b17 == 0 ) // the button in this row and current column is down
            {
                butstate[0] |= (1<<columns);
            }
            if( b18 == 0 ) // check the other row
            {
                butstate[1] |= (1<<columns);
            }
        }
    #else
        // fill out the states the same way
        if( !digitalRead(17) ) { butstate[1] |= (1 << JOY_BUT); }
        if( !digitalRead(18) ) { butstate[1] |= (1 << JOY_UP); }
        if( !digitalRead(26) ) { butstate[1] |= (1 << JOY_DOWN); }
        if( !digitalRead(27) ) { butstate[1] |= (1 << JOY_LEFT); }
        if( !digitalRead(28) ) { butstate[1] |= (1 << JOY_RIGHT); }
    #endif
}

float angle = 0.0f;


void setup() {
    Serial.begin(9600);
    Serial.println("starting on Pico");
    tft.init();
    tft.initDMA();
    generate_sintab();
    
    #ifdef VECTORSCOPE_BADGE
        // pins for bitbanging the joystick interface
        pinMode(16,OUTPUT);
        pinMode(0,OUTPUT);
        pinMode(1,OUTPUT);

        pinMode(17,INPUT);
        pinMode(18,INPUT);

        pinMode(22,OUTPUT); // Speaker
        digitalWrite(22,1); // Turn it OFF!
    #else
        pinMode(17,INPUT_PULLUP);
        pinMode(18,INPUT_PULLUP);
        pinMode(26,INPUT_PULLUP);
        pinMode(27,INPUT_PULLUP);
        pinMode(28,INPUT_PULLUP);
    #endif

    #ifdef TFT_BL
        pinMode(TFT_BL, OUTPUT);
    //    digitalWrite(TFT_BL, 1);
        analogWrite(TFT_BL, 255*2.5);
    #endif
}

unsigned long last_t = 0; // for delta calc

// some rotation variables
float logo_angle = 0.0f;
float wave_angle = 0.0f;
float rotspeed = 0;
float rotspeedang = 0;

int which = 0;
float message_speeds[]={-10,-8,-6,10};
BUTTONS but_last = BUT_INVALID;     // used for some button debouncing
    
void loop() {
    for( ;; )
    {
        unsigned long now = millis();
        unsigned long dt = now-last_t;
        last_t = now;
        if( dt > 1000 ){ dt = 20; }

        float delta = dt/1000.0f; // how long since the last update in seconds

        // increment some angle values
        wave_angle += delta*10.0f;
        fmodf(wave_angle,PI*2); // make sure they stay in range
        logo_angle += delta*rotspeed;
        rotspeed = sin(rotspeedang)*180.0f;
        rotspeedang += delta*PI*(PI/10);
        fmodf(rotspeedang,PI*2);

        // clear the backbuffer
        clear_buffer();
        // draw on the backbuffer
        // write_string_amiga( 119+sin(wave_angle*0.5)*45,119, "HACKADAY!", GREEN, true); // specify a color to overwrite the wood texture
        write_string_amiga( 119+sin(wave_angle*0.5)*45,119, "HACKADAY!", 0, true);
        write_curved_pac_string( 119, 119, "HACKADAY SUPERCON...", WHITE, wave_angle*message_speeds[0], 110 );
        write_curved_pac_string( 119, 119, "VECTORSCOPE DEMO", MAGENTA, wave_angle*message_speeds[1], 95 );
        write_curved_pac_string( 119, 119, "INSTANT ARCADE", YELLOW, wave_angle*message_speeds[2], 80 );
        write_curved_pac_string( 119, 119, "--HACK ME!--", CYAN, wave_angle*message_speeds[3], 55 );

        // allow user to change message speeds
        read_but_state();
        if( butstate[0] + butstate[1] == 0 ) but_last = BUT_INVALID; // clear debounce if no buttons pressed

        if( butstate[1] & (1<<JOY_LEFT) ) // decrease the speed of the current string is joy is to the left
        {
            message_speeds[which] -= 0.2f;
        }
        else if( butstate[1] & (1<<JOY_RIGHT) ) // increase the speed of the current string is joy is to the right
        {
            message_speeds[which] += 0.2f;
        }

        if( butstate[1] & (1<<JOY_UP) && but_last != JOY_UP) // move string selection up
        {
            if( which > 0 ){ which--; }
            but_last = JOY_UP;
        }

        if( butstate[1] & (1<<JOY_DOWN) && but_last != JOY_DOWN) // move string selection down
        {
            if( which < 3 ){ which++; }
            but_last = JOY_DOWN;
        }

        if( butstate[0] & (1<<BUT_MENU) && but_last != BUT_MENU) // toggle string rotation direction (Note Menu key is on the 0 row)
        {
            message_speeds[which] = -message_speeds[which];
            but_last = BUT_MENU;
        }

        // set leds to show selected text
        leds = 0; // clear them all first
        byte mapwhich[] = { BIT_SINE, BIT_SQUARE, BIT_TRIANGLE, BIT_SAW };
        set_led(mapwhich[which]);

        // sling the backbuffer to the display
        buffer_to_screen();
    }
}
