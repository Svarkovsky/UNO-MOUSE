/*  
 based on pollserial.h and PS2Keyboard.h
 Combined by Yotson, october 2011.
   
 ps2 keyboard library which uses the usart hardware for buffering individual bits into a byte before reading the byte by software.
 Goal was to use a ps2 keyboard in combination with tv-out without image/timing corruption. 
 The interrupt driven version of ps2 keyboard messed with tv out image/timing.
   
 Tested on atmega328. Not compatible with arduino mega/mega2560 as the XCKn pins are not broken out to header/pad.
   
 ps2-Keyboard pin   |  Arduino pin
 -------------------------------------------
 data               |  RX, digital pin 0
 clock              |  XCK, digital pin 4
 ground             |  Ground
 5V                 |  5V   
*/

#ifndef PS2UART_h
#define PS2UART_h

#if defined(ARDUINO) && ARDUINO >= 100
#include "Arduino.h"     
#else
//#include "WProgram.h"
#endif

// Every call to read() returns a single byte for each
// keystroke.  These configure what byte will be returned
// for each "special" key.  To ignore a key, use zero.
#define PS2_TAB         9
#define PS2_ENTER       13
#define PS2_BACKSPACE   127  // 0x7F
#define PS2_ESC         27
#define PS2_INSERT      0
#define PS2_DELETE      255  // 0xFF (unsigned to avoid sign extension)
#define PS2_HOME        230  // 0xE6 (unsigned)
#define PS2_END         233  // 0xE9 (unsigned)
#define PS2_PAGEUP      25
#define PS2_PAGEDOWN    26
#define PS2_UPARROW     11
#define PS2_LEFTARROW   8
#define PS2_DOWNARROW   10
#define PS2_RIGHTARROW  21
#define PS2_F1          0
#define PS2_F2          0
#define PS2_F3          0
#define PS2_F4          0
#define PS2_F5          0
#define PS2_F6          0
#define PS2_F7          0
#define PS2_F8          0
#define PS2_F9          0
#define PS2_F10         0
#define PS2_F11         0
#define PS2_F12         0
#define PS2_SCROLL      0

/* Previous versions of this library returned a mix of ascii characters
 * and raw scan codes (if no ASCII character could be found).  There was
 * no way to determine if a byte was ascii or a raw scan code.  With
 * version 2.0, only ASCII is returned, plus the special bytes above.
 *
 * PS2 keyboard "make" codes to check for certain keys.
 */
#define PS2_KC_BREAK  0xf0
#define PS2_KC_ENTER  0x0d
#define PS2_KC_ESC    0x76
#define PS2_KC_KPLUS  0x79
#define PS2_KC_KMINUS 0x7b
#define PS2_KC_KMULTI 0x7c
#define PS2_KC_NUM    0x77
#define PS2_KC_BKSP   0x66

//define a void function() return type.
typedef void (*pt2Funct)();

/**
 * Purpose: Provides an easy access to PS2 keyboards
 * Author:  Christian Weichel
 *
 * Altered by Yotson for usage with usart hardware, october 2011.
 */
class PS2uartKeyboard {
  public:    
     /**
      * Starts the keyboard "service".
      * setting the usart mode correctly.
      * Propably the best place to call this method is in the setup routine.
      */
    pt2Funct begin();

    /**
     * Returns true if there is a char to be read, false if not.
     */   
    bool available();
 
    /**
     * Returns the char last read from the keyboard.
     * If there is no char availble, -1 is returned.
     */   
    int read();
    
    /**
     * Clears buffer. 
     * Use this if you want to ignore all keys pressed uptil this moment.
     */
    void flush(void);
    
    private:

};

void USART_recieve();

#endif