#include "PS2uartKeyboard.h"
#include <avr/pgmspace.h>

#define UARTBUFFER_SIZE 16      // 45 байт избыточен
static uint8_t buffer[UARTBUFFER_SIZE];
static uint8_t head, tail;
static unsigned char ps2Keyboard_CharBuffer=0; // Изменено на unsigned char

void USART_recieve() {
#if defined ( UDR0 )
  if( UCSR0A & _BV(RXC0)) {
    uint8_t i = head + 1;
    if (i >= UARTBUFFER_SIZE) i = 0;
    if (i != tail) {
      buffer[i] = UDR0;
      head = i;
    }
  }
#else
  if( UCSRA & _BV(RXC)) {
    uint8_t i = head + 1;
    if (i >= UARTBUFFER_SIZE) i = 0;
    if (i != tail) {
      buffer[i] = UDR;
      head = i;
    }
  }
#endif
}

static inline uint8_t get_scan_code(void)
{
  uint8_t c, i;

  i = tail;
  if (i == head) return 0;
  i++;
  if (i >= UARTBUFFER_SIZE) i = 0;
  c = buffer[i];
  tail = i;
  return c;
}

// http://www.quadibloc.com/comp/scan.htm
// http://www.computer-engineering.org/ps2keyboard/scancodes2.html
#define BREAK     0x01
#define MODIFIER  0x02
#define SHIFT_L   0x04
#define SHIFT_R   0x08

// These arrays provide a simple key map, to turn scan codes into ASCII
// output.  If a non-US keyboard is used, these may need to be modified
// for the desired output.
static unsigned char __attribute((section(".progmem.data"))) scan2ascii_noshift[] = {
  0, PS2_F9, 0, PS2_F5, PS2_F3, PS2_F1, PS2_F2, PS2_F12,
  0, PS2_F10, PS2_F8, PS2_F6, PS2_F4, PS2_TAB, '`', 0,
  0, 0 /*Lalt*/, 0 /*Lshift*/, 0, 0 /*Lctrl*/, 'q', '1', 0,
  0, 0, 'z', 's', 'a', 'w', '2', 0,
  0, 'c', 'x', 'd', 'e', '4', '3', 0,
  0, ' ', 'v', 'f', 't', 'r', '5', 0,
  0, 'n', 'b', 'h', 'g', 'y', '6', 0,
  0, 0, 'm', 'j', 'u', '7', '8', 0,
  0, ',', 'k', 'i', 'o', '0', '9', 0,
  0, '.', '/', 'l', ';', 'p', '-', 0,
  0, 0, '\'', 0, '[', '=', 0, 0,
  0 /*CapsLock*/, 0 /*Rshift*/, PS2_ENTER /*Enter*/, ']', 0, '\\', 0, 0,
  0, 0, 0, 0, 0, 0, PS2_BACKSPACE, 0,
  0, '1', 0, '4', '7', 0, 0, 0,
  '0', '.', '2', '5', '6', '8', PS2_ESC, 0 /*NumLock*/,
  PS2_F11, '+', '3', '-', '*', '9', PS2_SCROLL, 0,
  0, 0, 0, PS2_F7 };

static unsigned char __attribute((section(".progmem.data"))) scan2ascii_shift[] = {
  0, PS2_F9, 0, PS2_F5, PS2_F3, PS2_F1, PS2_F2, PS2_F12,
  0, PS2_F10, PS2_F8, PS2_F6, PS2_F4, PS2_TAB, '~', 0,
  0, 0 /*Lalt*/, 0 /*Lshift*/, 0, 0 /*Lctrl*/, 'Q', '!', 0,
  0, 0, 'Z', 'S', 'A', 'W', '@', 0,
  0, 'C', 'X', 'D', 'E', '$', '#', 0,
  0, ' ', 'V', 'F', 'T', 'R', '%', 0,
  0, 'N', 'B', 'H', 'G', 'Y', '^', 0,
  0, 0, 'M', 'J', 'U', '&', '*', 0,
  0, '<', 'K', 'I', 'O', ')', '(', 0,
  0, '>', '?', 'L', ':', 'P', '_', 0,
  0, 0, '"', 0, '{', '+', 0, 0,
  0 /*CapsLock*/, 0 /*Rshift*/, PS2_ENTER /*Enter*/, '}', 0, '|', 0, 0,
  0, 0, 0, 0, 0, 0, PS2_BACKSPACE, 0,
  0, '1', 0, '4', '7', 0, 0, 0,
  '0', '.', '2', '5', '6', '8', PS2_ESC, 0 /*NumLock*/,
  PS2_F11, '+', '3', '-', '*', '9', PS2_SCROLL, 0,
  0, 0, 0, PS2_F7 };

static unsigned char get_ascii_code(void) // Изменено на unsigned char
{
  static uint8_t state=0;
  uint8_t s;
  unsigned char c; // Изменено на unsigned char

  while (1) {
    s = get_scan_code();

    if (!s) return 0;
    if (s == 0xF0) {
      state |= BREAK;
    } 
    else if (s == 0xE0) {
      state |= MODIFIER;
    } 
    else {
      if (state & BREAK) {
        if (s == 0x12) {
          state &= ~SHIFT_L;
        } 
        else if (s == 0x59) {
          state &= ~SHIFT_R;
        }
        state &= ~(BREAK | MODIFIER);
        continue;
      }
      if (s == 0x12) {
        state |= SHIFT_L;
        continue;
      } 
      else if (s == 0x59) {
        state |= SHIFT_R;
        continue;
      }
      c = 0;
      if (state & MODIFIER) {
        switch (s) {
        case 0x70: 
          c = PS2_INSERT;      
          break;
        case 0x6C: 
          c = PS2_HOME;        
          break;
        case 0x7D: 
          c = PS2_PAGEUP;      
          break;
        case 0x71: 
          c = PS2_DELETE;      
          break;
        case 0x69: 
          c = PS2_END;         
          break;
        case 0x7A: 
          c = PS2_PAGEDOWN;    
          break;
        case 0x75: 
          c = PS2_UPARROW;     
          break;
        case 0x6B: 
          c = PS2_LEFTARROW;   
          break;
        case 0x72: 
          c = PS2_DOWNARROW;   
          break;
        case 0x74: 
          c = PS2_RIGHTARROW;  
          break;
        case 0x4A: 
          c = '/';             
          break;
        case 0x5A: 
          c = PS2_ENTER;       
          break;
        default: 
          break;
        }
      } 
      else if (state & (SHIFT_L | SHIFT_R)) {
        if (s < sizeof(scan2ascii_shift))
          c = pgm_read_byte(scan2ascii_shift + s);
      } 
      else {
        if (s < sizeof(scan2ascii_noshift))
          c = pgm_read_byte(scan2ascii_noshift + s);
      }
      state &= ~(BREAK | MODIFIER);
      if (c) return c;
    }
  }
}

bool PS2uartKeyboard::available(){
  if (ps2Keyboard_CharBuffer) return true;
  ps2Keyboard_CharBuffer = get_ascii_code();
  if (ps2Keyboard_CharBuffer) return true;
  return false;
}

int PS2uartKeyboard::read(){
  unsigned char result = ps2Keyboard_CharBuffer; // Изменено на unsigned char
  if (result) {
    ps2Keyboard_CharBuffer = 0;
  } 
  else {
    result = get_ascii_code();
  }
  if (!result) return -1;
  return result;
}

void PS2uartKeyboard::flush(){
  head = tail;
}

pt2Funct PS2uartKeyboard::begin(void) {
  uint16_t baud_setting;
  bool use_u2x;
  long baud = 19200L;

  if (baud > F_CPU / 16) {
    use_u2x = true;
  }
  else {
    uint8_t nonu2x_baud_error = abs((int)(255-((F_CPU/(16*(((F_CPU/8/baud-1)/2)+1))*255)/baud)));
    uint8_t u2x_baud_error = abs((int)(255-((F_CPU/(8*(((F_CPU/4/baud-1)/2)+1))*255)/baud)));
    use_u2x = (nonu2x_baud_error > u2x_baud_error);
  }
  if (use_u2x) {
#if defined ( UDR0 )
    UCSR0A = _BV(U2X0);
#else
    UCSRA = _BV(U2X);
#endif
    baud_setting = (F_CPU / 4 / baud - 1) / 2;
  }
  else {
#if defined ( UDR0 )
    UCSR0A = 0;
#else
    UCSRA = 0;
#endif
    baud_setting = (F_CPU / 8 / baud - 1) / 2;
  }

#if defined ( UDR0 )
  UBRR0 = baud_setting;
  UCSR0B = _BV(RXEN0) | _BV(TXEN0);
  bitSet(UCSR0C, UMSEL00);
  bitSet(UCSR0C, UPM01);
  bitSet(UCSR0C, UPM00);
#else
  UBRR = baud_setting;
  UCSRB = _BV(RXEN) | _BV(TXEN);
  bitSet(UCSRC, UMSEL0);
  bitSet(UCSRC, UPM1);
  bitSet(UCSRC, UPM0);
#endif

  head = 0;
  tail = 0;
  return &USART_recieve;
}