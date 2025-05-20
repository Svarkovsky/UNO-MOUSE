// UNO MOUSE v0.1 (2025 05 20)

/*
MIT License

Copyright (c) Ivan Svarkovsky - 2025

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to whom the Software is furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR_THE USE OR OTHER DEALINGS IN THE
SOFTWARE.
*/

#include <TVout.h>
#include <PS2uartKeyboard.h>
#include <fontALL.h>
// Removed: #include <ctype.h> // Replaced with my_isdigit, my_isprint, my_isspace, my_toupper
#include <avr/pgmspace.h> // Needed for pgm_read_byte_near, pgm_read_ptr
#include <EEPROM.h>
// Removed: #include <stdlib.h> // Replaced with my_atol, my_itoa (for int8_t)
#include <avr/io.h>
// Removed: #include <string.h> // Replaced with custom my_ functions below

// --- Custom Function Implementations (More Economical Versions) ---
// Using inline for small functions to potentially reduce call overhead

// Аналог isdigit - компактная версия
inline bool my_isdigit(char c) {
    return (unsigned char)(c - '0') <= 9;
}

// Аналог isprint - компактная версия
inline bool my_isprint(char c) {
    return (unsigned char)(c - ' ') <= ('~' - ' ');
}

// Аналог isspace - компактная версия (включает \t, \n, \v, \f, \r)
inline bool my_isspace(char c) {
    return c == ' ' || (unsigned char)(c - '\t') <= ('\r' - '\t');
}

// Аналог toupper - компактная версия
inline char my_toupper(char c) {
    return (c >= 'a' && c <= 'z') ? c - 32 : c;
}

// Аналог atol - оптимизированная версия (используем int16_t для экономии RAM, т.к. числа Mouse в диапазоне int8_t)
inline int16_t my_atol(const char* str) {
    int16_t res = 0; // Use int16_t instead of long
    bool neg = *str == '-';
    if (*str == '-' || *str == '+') str++;

    // Use the compact isdigit check directly
    while ((unsigned char)(*str - '0') <= 9)
        res = res * 10 + (*str++ - '0'); // Compiler will optimize * 10

    return neg ? -res : res;
}

// Аналог itoa - оптимизированная версия для int8_t
inline void my_itoa(int8_t val, char* buf) { // Removed base parameter as it's always 10
    // Corrected buffer size to 5 for "-128\0"
    char tmp[5]; // Use a small temporary buffer on stack
    char* p = tmp;
    uint8_t v = val < 0 ? -val : val; // Safe for int8_t range

    // Generate digits in reverse order
    do {
        *p++ = v % 10 + '0';
    } while (v /= 10);

    // Add sign if negative
    if (val < 0) *buf++ = '-';

    // Copy digits from temporary buffer to destination buffer in correct order
    while (p > tmp) { // Loop while p is still pointing within tmp
        *buf++ = *(--p); // Decrement p first, then dereference
    }

    *buf = '\0'; // Null terminate
}

// Аналог strlen_P - оптимизированная версия для PROGMEM строк
inline uint16_t my_strlen_P(const char* str_P) {
    uint16_t len = 0;
    while (pgm_read_byte_near(str_P + len)) len++; // Direct check for non-zero byte
    return len;
}

// --- Ultra-optimized String Functions (Replacing <string.h>) ---

// Аналог memset - самая простая реализация
void* my_memset(void* s, int c, size_t n) {
    uint8_t* p = (uint8_t*)s;
    while (n--) *p++ = (uint8_t)c;
    return s;
}

// Аналог strcmp - компактная реализация, возвращает int8_t для экономии RAM
int8_t my_strcmp(const char* s1, const char* s2) {
    while (*s1 && *s1 == *s2) s1++, s2++;
    // Возвращаем разницу символов. Приведение к unsigned char для корректной работы
    // со всеми возможными значениями char, если char знаковый.
    return *(const unsigned char*)s1 - *(const unsigned char*)s2;
}

// Аналог strlen - минимальная реализация, возвращает uint8_t (достаточно для MAX_LINE_LENGTH)
uint8_t my_strlen(const char* s) {
    uint8_t len = 0;
    while (*s++) len++; // Инкрементируем указатель и проверяем, был ли символ ненулевым
    return len;
}

// Аналог strcat - оптимизированная версия
char* my_strcat(char* dest, const char* src) {
    char* d = dest;
    while (*d) d++;       // Find end of dest
    while ((*d++ = *src++)); // Copy src including null terminator
    return dest;
}

// Дополнительная рекомендация: Оптимизированное сравнение на равенство
// Может быть чуть быстрее, чем my_strcmp == 0, если строки равны.
bool my_streq(const char* s1, const char* s2) {
    while (*s1 && *s1 == *s2) s1++, s2++;
    return *s1 == *s2; // Возвращает true, если оба указателя указывают на '\0'
}


// --- End Custom Function Implementations ---


// --- Константы для TVout ---
#define VIDEO_SYSTEM NTSC
#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 96
#define FONT_TYPE font4x6
#define CHAR_WIDTH 4
#define CHAR_HEIGHT 6
#define CHARS_PER_LINE (SCREEN_WIDTH / CHAR_WIDTH) // 32
#define MAX_LINE_LENGTH (CHARS_PER_LINE - 2) // 30
#define LINES_PER_SCREEN (SCREEN_HEIGHT / CHAR_HEIGHT) // 16
// Итоговый текстовый режим: 32x16 символов.

// --- Константы для звуков ---
#define TONE_DURATION_MS_SHORT 30
#define TONE_DURATION_MS_LONG 50
#define TONE_FREQ_KEY 800
#define TONE_FREQ_BACKSP 400
#define TONE_FREQ_ENTER 1200
#define TONE_FREQ_BUFFER_FULL 200

// Константы для мелодии приветствия
#define NOTE_C4  262
#define NOTE_E4  330
#define NOTE_G4  392
#define NOTE_C5  523
#define MELODY_NOTE_DURATION 100

// --- Константы для курсора ---
#define CURSOR_BLINK_INTERVAL 500   // 500

// --- Константы для EEPROM ---
#define MAX_LINES 31
#define EEPROM_BLOCK_START 0
#define EEPROM_BLOCK_SIZE (MAX_LINES * (MAX_LINE_LENGTH + 1)) // 961 байт
#define EEPROM_MACRO_LINES_START (EEPROM_BLOCK_START + EEPROM_BLOCK_SIZE) // 961
#define EEPROM_MACRO_MAGIC_ADDR EEPROM_MACRO_LINES_START // 961
#define EEPROM_MACRO_MAGIC_VALUE 0xAF
#define EEPROM_MACRO_DATA_START (EEPROM_MACRO_MAGIC_ADDR + 1) // 962

// --- Константы для Mouse ---
#define STACK_SIZE 20
#define MAX_NESTING 3
#define MAX_MACROS 26

// --- Строки в PROGMEM ---
const char welcome_msg[] PROGMEM = "UNO MOUSE";
const char separator[] PROGMEM = "---------";
const char ok_msg[] PROGMEM = "OK";
const char syntax_error_msg[] PROGMEM = "SYNTAX ERROR";
const char program_ended_msg[] PROGMEM = "PROGRAM ENDED";
const char press_any_key_msg[] PROGMEM = "PRESS ANY KEY...";
const char unmatched_bracket_msg[] PROGMEM = "UNMATCHED BRACKET";
const char stack_overflow_msg[] PROGMEM = "STACK OVERFLOW";
const char call_stack_overflow_msg[] PROGMEM = "CALL STACK OVERFLOW";
const char call_stack_underflow_msg[] PROGMEM = "CALL STACK UNDERFLOW";
const char break_outside_loop_msg[] PROGMEM = "BREAK OUTSIDE LOOP";
const char undefined_macro_msg[] PROGMEM = "UNDEFINED MACRO";
const char invalid_macro_call_msg[] PROGMEM = "INVALID MACRO CALL";
const char test_loaded_msg[] PROGMEM = "TEST LOADED";

// --- TEST PROGRAM STORED IN PROGMEM ---
const char test_program_flash[] PROGMEM =
"E\n"                    // Очистка экрана
"30 40 25 C\n"           // Круг: центр (30,40), радиус 25 (незалитый по умолчанию)
"10 15 10 50 40 50 T\n"  // Треугольник: вершины (30,15), (10,60), (50,60) - вписан в круг
"60 20 40 40 R\n"        // Внешний квадрат: угол (60,20), размер 40x40 (контур)
"1 F 70 30 20 20 R\n"    // Внутренний квадрат: угол (70,30), размер 20x20 (залитый) - в два раза меньше и центрирован
"110 10 110 70 L\n"      // Вертикальная линия 1
"112 10 112 70 L\n"      // Вертикальная линия 2
"114 10 114 70 L\n"      // Вертикальная линия 3
"116 10 116 70 L\n"      // Вертикальная линия 4
"118 10 118 70 L\n"      // Вертикальная линия 5
"$$\n"                   // Конец программы
"RUN\n"
;


// --- Глобальные объекты ---
TVout TV;
PS2uartKeyboard keyboard;

// --- Глобальные переменные ---
struct {
  uint8_t cursor_visible : 1;
  uint8_t running : 1;
  uint8_t tracing : 1;
  uint8_t stack_overflow : 1;
} flags = {0, 0, 0, 0};
unsigned long last_blink = 0;
char line_buffer[MAX_LINE_LENGTH + 1] = {0};
uint8_t buffer_len = 0;
uint8_t current_line = 0;
uint8_t cursor_x = 0;
uint8_t cursor_y = 0;
char input_buffer[10];
uint8_t input_buffer_ptr = 0;

// --- Mouse интерпретатор ---
int8_t stack[STACK_SIZE];
uint8_t stack_ptr = 0;
int8_t vars[26];
uint8_t call_stack[MAX_NESTING][2];
uint8_t call_stack_ptr = 0;
static int8_t fill_flag = 0; // Инициализируем флаг заливки в 0 (без заливки)

// --- Таблица команд в PROGMEM ---
typedef void (*cmd_func_t)();
#define CMD(c, f) {c, f}
void cmd_add(); void cmd_sub(); void cmd_mul(); void cmd_div(); void cmd_mod();
void cmd_assign(); void cmd_var_get(); void cmd_lt(); void cmd_gt();
void cmd_print_num(); void cmd_input();
void cmd_trace_on(); void cmd_trace_off();
void cmd_set_pixel(); void cmd_draw_line(); void cmd_draw_circle();
void cmd_draw_rect(); void cmd_set_fill(); void cmd_clear_screen();
void cmd_draw_triangle(); // Новая команда: рисование треугольника

const struct {
  char cmd;
  cmd_func_t func;
} cmd_table[] PROGMEM = {
  CMD('+', cmd_add),
  CMD('-', cmd_sub),
  CMD('*', cmd_mul),
  CMD('/', cmd_div),
  CMD('\\', cmd_mod),
  CMD('=', cmd_assign),
  CMD('.', cmd_var_get),
  CMD('<', cmd_lt),
  CMD('>', cmd_gt),
  CMD('!', cmd_print_num),
  CMD('?', cmd_input),
  CMD('{', cmd_trace_on),
  CMD('}', cmd_trace_off),
  CMD('P', cmd_set_pixel),
  CMD('L', cmd_draw_line),
  CMD('C', cmd_draw_circle),
  CMD('R', cmd_draw_rect),
  CMD('F', cmd_set_fill),
  CMD('E', cmd_clear_screen),
  CMD('T', cmd_draw_triangle) // Добавлена команда T
};

// --- Функции команд ---
void push(int8_t v) {
    if (stack_ptr < STACK_SIZE) {
        stack[stack_ptr++] = v;
    } else {
        flags.stack_overflow = 1;
    }
}
int8_t pop() { return stack_ptr > 0 ? stack[--stack_ptr] : 0; }

void cmd_add() { if (stack_ptr >= 2) { int8_t b = pop(); push(pop() + b); } else { push(0); } }
void cmd_sub() { if (stack_ptr >= 2) { int8_t b = pop(); push(pop() - b); } else { push(0); } }
void cmd_mul() { if (stack_ptr >= 2) { int8_t b = pop(); push(pop() * b); } else { push(0); } }
void cmd_div() { if (stack_ptr >= 2) { int8_t b = pop(); if (b != 0) push(pop() / b); else push(0); } else { push(0); } }
void cmd_mod() { if (stack_ptr >= 2) { int8_t b = pop(); if (b != 0) push(pop() % b); else push(0); } else { push(0); } }

void cmd_assign() {
  if (stack_ptr >= 2) {
    int8_t v = pop();
    int8_t a = pop();
    if (a >= 0 && a < 26) {
        vars[a] = v;
    }
  } else if (stack_ptr == 1) {
      int8_t a = pop();
      if (a >= 0 && a < 26) vars[a] = 0;
  }
}

void cmd_var_get() { if (stack_ptr > 0) { int8_t a = pop(); if (a >= 0 && a < 26) push(vars[a]); else push(0); } else { push(0); } }

void cmd_lt() { if (stack_ptr >= 2) { int8_t b = pop(); push(pop() < b); } else { push(0); } }
void cmd_gt() { if (stack_ptr >= 2) { int8_t b = pop(); push(pop() > b); } else { push(0); } }

void cmd_print_num() {
  if (stack_ptr > 0) {
    int8_t value = pop();
    char num_str[5];
    // Use custom itoa for int8_t
    my_itoa(value, num_str); // Removed base 10 argument
    TV.print(num_str);
    if (flags.tracing) {
      TV.println();
      print_stack_content();
      TV.print(" (after !)");
    }
  }
}

void cmd_input() {
    TV.print("? ");
    input_buffer_ptr = 0;
    input_buffer[0] = 0;
    bool reading = true;
    while(reading) {
        while(!keyboard.available());
        int k = keyboard.read();
        if (k == PS2_ENTER || k == '\r' || k == '\n') {
            reading = false;
            playTone(TONE_FREQ_ENTER, TONE_DURATION_MS_LONG);
        } else if (k == PS2_BACKSPACE) {
            if (input_buffer_ptr > 0) {
                input_buffer_ptr--;
                input_buffer[input_buffer_ptr] = 0;
                TV.print('\b');
                TV.print(' ');
                TV.print('\b');
                playTone(TONE_FREQ_BACKSP, TONE_DURATION_MS_SHORT);
            } else {
                 playTone(TONE_FREQ_BUFFER_FULL, TONE_DURATION_MS_SHORT);
            }
        } else if (my_isprint(k)) { // Use custom isprint
            if (input_buffer_ptr < sizeof(input_buffer) - 1) {
                if (k == '-' && input_buffer_ptr > 0) {
                     playTone(TONE_FREQ_BUFFER_FULL, TONE_DURATION_MS_SHORT);
                     continue;
                }
                if (my_isdigit(k) || k == '-') { // Use custom isdigit
                    input_buffer[input_buffer_ptr++] = k;
                    input_buffer[input_buffer_ptr] = 0;
                    TV.print((char)k);
                    playTone(TONE_FREQ_KEY, TONE_DURATION_MS_SHORT);
                } else {
                     playTone(TONE_FREQ_BUFFER_FULL, TONE_DURATION_MS_SHORT);
                }
            } else {
                playTone(TONE_FREQ_BUFFER_FULL, TONE_DURATION_MS_SHORT);
            }
        }
    }
    TV.println();
    long value = my_atol(input_buffer); // Use custom atol (returns int16_t)
    if (value >= -128 && value <= 127) {
        push((int8_t)value);
    } else {
        push(0); // Push 0 if value is out of int8_t range
    }
}

void cmd_trace_on() { flags.tracing = 1; }
void cmd_trace_off() { flags.tracing = 0; }

void print_stack_content() {
    TV.print('[');
    for (uint8_t i = 0; i < stack_ptr; ++i) {
        char num_str[5];
        // Use custom itoa for int8_t
        my_itoa(stack[i], num_str); // Removed base 10 argument
        TV.print(num_str);
        if (i < stack_ptr - 1) {
            TV.print(' ');
        }
    }
    TV.print(']');
}

// --- Новые графические команды ---
void cmd_set_pixel() {
    if (stack_ptr >= 2) {
        int8_t y = pop();
        int8_t x = pop();
        if (x >= 0 && x < SCREEN_WIDTH && y >= 0 && y < SCREEN_HEIGHT) {
            TV.set_pixel(x, y, 1); // Белый пиксель
        }
    }
}

void cmd_draw_line() {
    if (stack_ptr >= 4) {
        int8_t y2 = pop();
        int8_t x2 = pop();
        int8_t y1 = pop();
        int8_t x1 = pop();
        // Проверка границ (опционально, но рекомендуется)
        if (x1 >= 0 && x1 < SCREEN_WIDTH && y1 >= 0 && y1 < SCREEN_HEIGHT &&
            x2 >= 0 && x2 < SCREEN_WIDTH && y2 >= 0 && y2 < SCREEN_HEIGHT) {
            TV.draw_line(x1, y1, x2, y2, 1); // Цвет 1 (белый)
        }
    }
}

void cmd_draw_circle() {
    if (stack_ptr >= 3) {
        int8_t r = pop();
        int8_t y = pop();
        int8_t x = pop();
        // Проверка границ и радиуса
        if (x >= 0 && x < SCREEN_WIDTH && y >= 0 && y < SCREEN_HEIGHT && r >= 0) {
            TV.draw_circle(x, y, r, 1, fill_flag); // Цвет 1 (белый), используем fill_flag
        }
    }
    fill_flag = 0; // Сброс флага заливки после рисования
}

void cmd_draw_rect() {
    if (stack_ptr >= 4) {
        int8_t h = pop();
        int8_t w = pop();
        int8_t y = pop();
        int8_t x = pop();
        // Проверка границ и размеров
        if (x >= 0 && x < SCREEN_WIDTH && y >= 0 && y < SCREEN_HEIGHT && w > 0 && h > 0) {
            TV.draw_rect(x, y, w, h, 1, fill_flag); // Цвет 1 (белый), используем fill_flag
        }
    }
    fill_flag = 0; // Сброс флага заливки после рисования
}

void cmd_set_fill() {
    if (stack_ptr >= 1) {
        // Если значение со стека не равно нулю (истина), устанавливаем fill_flag в 1 (заливка).
        // Если значение со стека равно нулю (ложь), устанавливаем fill_flag в 0 (без заливки).
        fill_flag = pop() ? 1 : 0;
    }
}

void cmd_clear_screen() {
    TV.clear_screen(); // Очистка всего экрана
}

// Добавлена команда для рисования треугольника
void cmd_draw_triangle() {
    if (stack_ptr >= 6) {
        // Параметры на стеке: x1 y1 x2 y2 x3 y3 T
        int8_t y3 = pop();
        int8_t x3 = pop();
        int8_t y2 = pop();
        int8_t x2 = pop();
        int8_t y1 = pop();
        int8_t x1 = pop();

        // Проверка границ (опционально)
        // Можно добавить проверку всех 6 координат, но для простоты пропустим
        // и положимся на TV.draw_line, которая может обрезать линии.

        // Рисуем три линии, соединяющие вершины
        TV.draw_line(x1, y1, x2, y2, 1); // Линия 1-2
        TV.draw_line(x2, y2, x3, y3, 1); // Линия 2-3
        TV.draw_line(x3, y3, x1, y1, 1); // Линия 3-1

        // Треугольник, нарисованный линиями, не использует fill_flag
        // Если нужна заливка, это потребует отдельного алгоритма (более сложного и затратного по RAM)
    }
}


// --- Вспомогательные функции ---
void playTone(int freq, int duration) {
  TV.tone(freq, duration);
  delay(duration + 5);
  TV.noTone();
}

void playWelcomeMelody() {
  playTone(NOTE_C4, MELODY_NOTE_DURATION);
  playTone(NOTE_E4, MELODY_NOTE_DURATION);
  playTone(NOTE_G4, MELODY_NOTE_DURATION);
  playTone(NOTE_C5, MELODY_NOTE_DURATION * 2);
}

void updateCursorPosition() {
  TV.set_cursor(cursor_x * CHAR_WIDTH, cursor_y * CHAR_HEIGHT);
}

void drawCursor(bool show) {
  updateCursorPosition();
  if (show) {
    TV.print('_');
  } else {
    if (cursor_x > 0 && cursor_x - 1 < buffer_len) {
      TV.print(line_buffer[cursor_x - 1]);
    } else {
      TV.print(' ');
    }
  }
}

void handleCursorBlink() {
  unsigned long current_millis = millis();
  if (current_millis - last_blink >= CURSOR_BLINK_INTERVAL) {
    flags.cursor_visible = !flags.cursor_visible;
    drawCursor(flags.cursor_visible);
    last_blink = current_millis;
  }
}

void clearLineBuffer() {
  my_memset(line_buffer, 0, MAX_LINE_LENGTH + 1); // Использование my_memset
  buffer_len = 0;
}

void redrawScreenLine(uint8_t screen_y, bool is_active) {
  TV.set_cursor(0, screen_y * CHAR_HEIGHT);
  TV.print(is_active ? ">" : " ");
  for (int i = 0; i < buffer_len && i < MAX_LINE_LENGTH; i++) {
    TV.print(line_buffer[i]);
  }
  for (int i = buffer_len; i < MAX_LINE_LENGTH; i++) {
    TV.print(' ');
  }
  if (is_active) {
    updateCursorPosition();
  }
}

// --- Функции EEPROM для программы ---
void saveLineToEEPROM(uint8_t line_idx) {
  if (line_idx >= MAX_LINES) return;
  uint16_t offset = EEPROM_BLOCK_START + line_idx * (MAX_LINE_LENGTH + 1);
  for (uint8_t i = 0; i < MAX_LINE_LENGTH; i++) {
    EEPROM.update(offset + i, line_buffer[i]);
  }
  EEPROM.update(offset + MAX_LINE_LENGTH, buffer_len);
}

void loadLineFromEEPROM(uint8_t line_idx) {
  if (line_idx >= MAX_LINES) {
    clearLineBuffer();
    return;
  }
  uint16_t offset = EEPROM_BLOCK_START + line_idx * (MAX_LINE_LENGTH + 1);
  for (uint8_t i = 0; i < MAX_LINE_LENGTH; i++) {
    line_buffer[i] = EEPROM.read(offset + i);
  }
  line_buffer[MAX_LINE_LENGTH] = 0;
  buffer_len = EEPROM.read(offset + MAX_LINE_LENGTH);
  if (buffer_len > MAX_LINE_LENGTH) {
    buffer_len = 0;
    my_memset(line_buffer, 0, MAX_LINE_LENGTH); // Использование my_memset
  }
}

void redrawAllLines(uint8_t active_line) {
  TV.clear_screen();
  uint8_t start_line = (active_line / LINES_PER_SCREEN) * LINES_PER_SCREEN;
  for (uint8_t i = 0; i < LINES_PER_SCREEN && (start_line + i) < MAX_LINES; i++) {
    loadLineFromEEPROM(start_line + i);
    redrawScreenLine(i, (start_line + i) == active_line);
  }
  loadLineFromEEPROM(active_line);
  current_line = active_line;
  cursor_y = current_line % LINES_PER_SCREEN;
  updateCursorPosition();
}

// --- Функции EEPROM для макросов ---
void initMacroEEPROM() {
    uint8_t magic = EEPROM.read(EEPROM_MACRO_MAGIC_ADDR);
    if (magic != EEPROM_MACRO_MAGIC_VALUE) {
        for (uint8_t i = 0; i < MAX_MACROS; i++) {
            EEPROM.put(EEPROM_MACRO_DATA_START + i * 2, (uint16_t)0xFFFF);
        }
        EEPROM.update(EEPROM_MACRO_MAGIC_ADDR, EEPROM_MACRO_MAGIC_VALUE);
    }
}

void resetMacroEEPROM() {
    for (uint8_t i = 0; i < MAX_MACROS; i++) {
        EEPROM.put(EEPROM_MACRO_DATA_START + i * 2, (uint16_t)0xFFFF);
    }
}

uint16_t getMacroAddress(uint8_t macro_idx) {
    if (macro_idx >= MAX_MACROS) return 0xFFFF;
    uint16_t address;
    EEPROM.get(EEPROM_MACRO_DATA_START + macro_idx * 2, address);
    return address;
}

void setMacroAddress(uint8_t macro_idx, uint16_t address) {
    if (macro_idx >= MAX_MACROS) return;
    EEPROM.put(EEPROM_MACRO_DATA_START + macro_idx * 2, address);
}

void scanForMacros() {
    resetMacroEEPROM();
    char scan_buffer[MAX_LINE_LENGTH + 1];
    uint8_t scan_buffer_len;
    for (uint8_t line_idx = 0; line_idx < MAX_LINES; line_idx++) {
        uint16_t offset = EEPROM_BLOCK_START + line_idx * (MAX_LINE_LENGTH + 1);
        for (uint8_t i = 0; i < MAX_LINE_LENGTH; i++) {
            scan_buffer[i] = EEPROM.read(offset + i);
        }
        scan_buffer[MAX_LINE_LENGTH] = 0;
        scan_buffer_len = EEPROM.read(offset + MAX_LINE_LENGTH);
        if (scan_buffer_len > MAX_LINE_LENGTH) scan_buffer_len = 0;
        uint8_t scan_pc = 0;
        // Use custom isspace
        while(scan_pc < scan_buffer_len && my_isspace(scan_buffer[scan_pc])) {
            scan_pc++;
        }
        if (scan_pc + 1 < scan_buffer_len && scan_buffer[scan_pc] == '$') {
            char macro_letter = scan_buffer[scan_pc + 1];
            if (macro_letter >= 'A' && macro_letter <= 'Z') {
                uint8_t macro_idx = macro_letter - 'A';
                uint16_t macro_address = offset + scan_pc + 2;
                setMacroAddress(macro_idx, macro_address);
            }
        }
    }
}

void loadTestProgramFromFlash(const char* test_program_P) {
    for (uint16_t i = EEPROM_BLOCK_START; i < EEPROM_BLOCK_START + EEPROM_BLOCK_SIZE; i++) {
        EEPROM.update(i, 0);
    }
    resetMacroEEPROM();
    uint16_t flash_ptr = 0;
    uint8_t current_eeprom_line = 0;
    char temp_line_buffer[MAX_LINE_LENGTH + 1];
    uint8_t temp_buffer_ptr = 0;
    my_memset(temp_line_buffer, 0, MAX_LINE_LENGTH + 1); // Использование my_memset
    while (current_eeprom_line < MAX_LINES) {
        char c = pgm_read_byte(test_program_P + flash_ptr);
        if (c == 0) break;
        if (c == '\n' || temp_buffer_ptr >= MAX_LINE_LENGTH) {
            uint16_t eeprom_offset = EEPROM_BLOCK_START + current_eeprom_line * (MAX_LINE_LENGTH + 1);
            for (uint8_t i = 0; i < MAX_LINE_LENGTH; i++) {
                EEPROM.update(eeprom_offset + i, temp_line_buffer[i]);
            }
            EEPROM.update(eeprom_offset + MAX_LINE_LENGTH, temp_buffer_ptr);
            current_eeprom_line++;
            temp_buffer_ptr = 0;
            my_memset(temp_line_buffer, 0, MAX_LINE_LENGTH + 1); // Использование my_memset
            if (c == '\n') {
                 flash_ptr++;
            }
            if (temp_buffer_ptr == 0 && current_eeprom_line < MAX_LINES && pgm_read_byte(test_program_P + flash_ptr) == '\n') {
                 flash_ptr++;
            }
        } else {
            temp_line_buffer[temp_buffer_ptr++] = c;
            flash_ptr++;
        }
    }
    for (uint8_t i = current_eeprom_line; i < MAX_LINES; i++) {
        uint16_t eeprom_offset = EEPROM_BLOCK_START + i * (MAX_LINE_LENGTH + 1);
        for (uint8_t j = 0; j < MAX_LINE_LENGTH + 1; j++) {
            EEPROM.update(eeprom_offset + j, 0);
        }
    }
    showProgmemMessageAndRedraw(test_loaded_msg, 1000);
}

void showProgmemMessageAndRedraw(const char* message_P, int duration_ms) {
  drawCursor(false);
  TV.clear_screen();
  // Use custom strlen_P
  uint16_t msg_len = my_strlen_P(message_P);
  TV.set_cursor(((CHARS_PER_LINE - msg_len) / 2) * CHAR_WIDTH, (LINES_PER_SCREEN / 2) * CHAR_HEIGHT);
  const char* ptr = message_P;
  while(char c = pgm_read_byte(ptr++)) {
      TV.print(c);
    }
  delay(duration_ms);
  clearLineBuffer();
  cursor_x = 1;
  loadLineFromEEPROM(current_line);
  cursor_x = buffer_len + 1;
  redrawAllLines(current_line);
  last_blink = millis();
  flags.cursor_visible = true;
}

void saveAllLines() {
  showProgmemMessageAndRedraw(ok_msg, 500);
}

void eraseNonEmptyLines() {
  for (uint8_t i = 0; i < MAX_LINES; i++) {
    loadLineFromEEPROM(i);
    if (buffer_len > 0) {
      uint16_t offset = EEPROM_BLOCK_START + i * (MAX_LINE_LENGTH + 1);
      for (uint8_t j = 0; j < MAX_LINE_LENGTH + 1; j++) {
        EEPROM.update(offset + j, 0);
      }
    }
  }
  resetMacroEEPROM();
  current_line = 0;
  cursor_y = 0;
  showProgmemMessageAndRedraw(ok_msg, 500);
}

int getFreeRam() {
  extern int __heap_start, *__brkval;
  int v;
  return (int)&v - (__brkval == 0 ? (int)&__heap_start : (int)__brkval);
}

void handleFreeRamCommand() {
  int free_ram = getFreeRam();
  char ram_buffer[8]; // Достаточно для "30000b\0"
  // Keep standard itoa here as my_itoa is for int8_t
  itoa(free_ram, ram_buffer, 10);
  // Use custom strlen and strcat for RAM buffer
  my_strcat(ram_buffer, "b"); // Использование my_strcat
  drawCursor(false);
  TV.clear_screen();
  TV.set_cursor(((CHARS_PER_LINE - my_strlen(ram_buffer)) / 2) * CHAR_WIDTH, (LINES_PER_SCREEN / 2) * CHAR_HEIGHT); // Использование my_strlen
  TV.print(ram_buffer);
  delay(3000);
  clearLineBuffer();
  cursor_x = 1;
  loadLineFromEEPROM(current_line);
  cursor_x = buffer_len + 1;
  redrawAllLines(current_line);
  last_blink = millis();
  flags.cursor_visible = true;
}


bool executeCommand(uint8_t &pc, uint8_t line_idx) {
  if (pc >= buffer_len) return false;
  char c = line_buffer[pc];

  // 1. Игнорируем пробелы
  // Use custom isspace
  if (my_isspace(c)) {
    pc++;
    return true;
  }

  // 2. Обработка строк в кавычках
  if (c == '"') {
    pc++;
    while (pc < buffer_len && line_buffer[pc] != '"') {
      if (line_buffer[pc] == '!') TV.print('\n');
      else TV.print(line_buffer[pc]);
      pc++;
    }
    if (pc < buffer_len && line_buffer[pc] == '"') {
      pc++;
      return true;
    } else {
      return false; // Непарная кавычка
    }
  }

  // 3. Обработка чисел (включая отрицательные)
  // Проверяем, начинается ли с цифры или с минуса, за которым следует цифра
  // Use custom isdigit
  if (my_isdigit(c) || (c == '-' && pc + 1 < buffer_len && my_isdigit(line_buffer[pc + 1]))) {
    char num_buf[5] = {0};
    uint8_t i = 0;
    uint8_t start_pc = pc;
    if (c == '-') {
      num_buf[i++] = c;
      pc++;
      // Use custom isdigit
      if (pc >= buffer_len || !my_isdigit(line_buffer[pc])) {
          pc = start_pc; // Это был просто минус, не часть числа
      }
    }
    // Use custom isdigit
    if (pc < buffer_len && my_isdigit(line_buffer[pc])) {
        // Use custom isdigit
        while (pc < buffer_len && my_isdigit(line_buffer[pc])) {
          if (i < sizeof(num_buf) - 1) {
             num_buf[i++] = line_buffer[pc];
             pc++;
          } else {
             break;
          }
        }
        bool is_valid_number_start = (num_buf[0] != 0 && !(i == 1 && num_buf[0] == '-'));
        if (is_valid_number_start) {
            long value = my_atol(num_buf); // Use custom atol (returns int16_t)
            if (value >= -128 && value <= 127) {
                 push((int8_t)value);
                 return true; // Число успешно обработано
            } else {
                 pc = start_pc;
                 return false; // Число вне диапазона int8_t
            }
        } else {
             pc = start_pc; // Это был просто минус без цифр
        }
    } else {
        pc = start_pc; // Это был просто минус без цифр
    }
  }

  // 4. Обработка команд из таблицы cmd_table (ПЕРЕМЕЩЕНО ВЫШЕ)
  for (uint8_t i = 0; i < sizeof(cmd_table) / sizeof(cmd_table[0]); i++) {
    if (c == pgm_read_byte(&cmd_table[i].cmd)) {
      cmd_func_t func = (cmd_func_t)pgm_read_ptr(&cmd_table[i].func);
      func();
      pc++; // Advance pc after executing command
      return true; // Команда успешно обработана
    }
  }


  // 5. Обработка переменных (A-Z) 
  // Эта проверка теперь выполняется ТОЛЬКО если символ не был командой из таблицы
  if (c >= 'A' && c <= 'Z') {
    push(c - 'A'); // Помещаем индекс переменной в стек
    pc++;
    return true; // Переменная успешно обработана
  }


  // 6. Syntax error if not processed
  return false; // Неизвестный символ или синтаксическая ошибка
}



bool find_matching_bracket(uint8_t start_line, uint8_t start_pc, char open_char, char close_char, uint8_t &end_line, uint8_t &end_pc) {
    int nesting_level = 1;
    uint8_t current_line_idx = start_line;
    uint8_t current_pc = start_pc;
    char scan_buffer[MAX_LINE_LENGTH + 1];
    uint8_t scan_buffer_len;
    while (current_line_idx < MAX_LINES) {
        uint16_t offset = EEPROM_BLOCK_START + current_line_idx * (MAX_LINE_LENGTH + 1);
        for (uint8_t i = 0; i < MAX_LINE_LENGTH; i++) {
            scan_buffer[i] = EEPROM.read(offset + i);
        }
        scan_buffer[MAX_LINE_LENGTH] = 0;
        scan_buffer_len = EEPROM.read(offset + MAX_LINE_LENGTH);
        if (scan_buffer_len > MAX_LINE_LENGTH) scan_buffer_len = 0;
        uint8_t scan_pc = (current_line_idx == start_line) ? start_pc + 1 : 0;
        while (scan_pc < scan_buffer_len) {
            char c = scan_buffer[scan_pc];
            if (c == '\'') {
                scan_pc = scan_buffer_len;
                continue;
            }
            if (c == '$' && scan_pc + 1 < scan_buffer_len && scan_buffer[scan_pc+1] >= 'A' && scan_buffer[scan_pc+1] <= 'Z') {
                scan_pc += 2;
                continue;
            }
            if (c == '$' && scan_pc + 1 < buffer_len && buffer_len > 0 && scan_buffer[scan_pc+1] == '$') {
                scan_pc += 2;
                continue;
            }
            if (c == open_char) {
                nesting_level++;
            } else if (c == close_char) {
                nesting_level--;
                if (nesting_level == 0) {
                    end_line = current_line_idx;
                    end_pc = scan_pc;
                    return true;
                }
            }
            scan_pc++;
        }
        current_line_idx++;
    }
    return false;
}

void runProgram() {
  TV.clear_screen(); // Очистка экрана перед выполнением
  TV.set_cursor(0, 0);
  stack_ptr = 0;
  call_stack_ptr = 0;
  my_memset(vars, 0, sizeof(vars)); // Использование my_memset
  flags.running = 1;
  flags.tracing = 0;
  flags.stack_overflow = 0;
  fill_flag = 0; // Сброс флага заливки в начале выполнения программы
  bool error = false;
  uint8_t error_line = 0;
  const char* error_msg_P = nullptr;
  uint8_t exec_line_idx = 0;
  uint8_t pc = 0;
  scanForMacros();
  // Очищаем область вывода (строки 0 до LINES_PER_SCREEN - 3)
  for(uint8_t i = 0; i < LINES_PER_SCREEN - 3; ++i) {
      TV.set_cursor(0, i * CHAR_HEIGHT);
      for(int j=0; j<CHARS_PER_LINE; ++j) TV.print(' ');
  }
  TV.set_cursor(0, 0); // Возвращаем курсор в начало области вывода (строка 0)

  while (exec_line_idx < MAX_LINES && !error && flags.running) {
    loadLineFromEEPROM(exec_line_idx);
    if (buffer_len == 0) {
      exec_line_idx++;
      pc = 0;
      TV.println();
      continue;
    }
    while (pc < buffer_len && !error && flags.running) {
      if (line_buffer[pc] == '$') {
          if (pc + 1 < buffer_len) {
              char next_char = line_buffer[pc+1];
              if (next_char == '$') {
                  flags.running = 0;
                  pc += 2;
                  break;
              } else if (next_char >= 'A' && next_char <= 'Z') {
                  pc = buffer_len;
                  continue;
              }
          }
          pc++;
          continue;
      }
      if (line_buffer[pc] == '\'') {
          pc = buffer_len;
          continue;
      }
      char current_char = line_buffer[pc];
      bool tracing_was_on_before = flags.tracing;
      bool handled_special_command = false;
      if (current_char == '[') {
          handled_special_command = true;
          int8_t condition = pop();
          if (condition <= 0) {
              uint8_t end_line, end_pc;
              if (find_matching_bracket(exec_line_idx, pc, '[', ']', end_line, end_pc)) {
                  exec_line_idx = end_line;
                  pc = end_pc + 1;
              } else {
                  error = true;
                  error_line = exec_line_idx;
                  error_msg_P = unmatched_bracket_msg;
              }
          } else {
              pc++;
          }
      } else if (current_char == ']') {
          handled_special_command = true;
          pc++;
      } else if (current_char == '(') {
          handled_special_command = true;
          if (call_stack_ptr < MAX_NESTING) {
              call_stack[call_stack_ptr][0] = exec_line_idx;
              call_stack[call_stack_ptr][1] = pc;
              call_stack_ptr++;
              pc++;
          } else {
              error = true;
              error_line = exec_line_idx;
              error_msg_P = call_stack_overflow_msg;
          }
      } else if (current_char == ')') {
          handled_special_command = true;
          if (call_stack_ptr > 0) {
              call_stack_ptr--;
              exec_line_idx = call_stack[call_stack_ptr][0];
              pc = call_stack[call_stack_ptr][1];
              loadLineFromEEPROM(exec_line_idx);
          } else {
              error = true;
              error_line = exec_line_idx;
              error_msg_P = unmatched_bracket_msg;
          }
      } else if (current_char == '^') {
          handled_special_command = true;
          int8_t condition = pop();
          if (condition <= 0) {
              if (call_stack_ptr > 0) {
                  uint8_t loop_start_line = call_stack[call_stack_ptr-1][0];
                  uint8_t loop_start_pc = call_stack[call_stack_ptr-1][1];
                  uint8_t end_line, end_pc;
                  if (find_matching_bracket(loop_start_line, loop_start_pc, '(', ')', end_line, end_pc)) {
                       call_stack_ptr--;
                       exec_line_idx = end_line;
                       pc = end_pc + 1;
                       loadLineFromEEPROM(exec_line_idx);
                  } else {
                       error = true;
                       error_line = exec_line_idx;
                       error_msg_P = unmatched_bracket_msg;
                  }
              } else {
                  error = true;
                  error_line = exec_line_idx;
                  error_msg_P = break_outside_loop_msg;
              }
          } else {
              pc++;
          }
      } else if (current_char == '#') {
          handled_special_command = true;
          if (pc + 1 < buffer_len) {
              char macro_letter = line_buffer[pc + 1];
              if (macro_letter >= 'A' && macro_letter <= 'Z') {
                  uint8_t macro_idx = macro_letter - 'A';
                  uint16_t target_address = getMacroAddress(macro_idx);
                  if (target_address != 0xFFFF) {
                      if (call_stack_ptr < MAX_NESTING) {
                          call_stack[call_stack_ptr][0] = exec_line_idx;
                          call_stack[call_stack_ptr][1] = pc + 2;
                          call_stack_ptr++;
                          exec_line_idx = target_address / (MAX_LINE_LENGTH + 1);
                          pc = target_address % (MAX_LINE_LENGTH + 1);
                          loadLineFromEEPROM(exec_line_idx);
                      } else {
                          error = true;
                          error_line = exec_line_idx;
                          error_msg_P = call_stack_overflow_msg;
                      }
                  } else {
                      error = true;
                      error_line = exec_line_idx;
                      error_msg_P = undefined_macro_msg;
                  }
              } else {
                  error = true;
                  error_line = exec_line_idx;
                  error_msg_P = invalid_macro_call_msg;
              }
          } else {
              error = true;
              error_line = exec_line_idx;
              error_msg_P = invalid_macro_call_msg;
          }
      } else if (current_char == '@') {
          handled_special_command = true;
          if (call_stack_ptr > 0) {
              call_stack_ptr--;
              exec_line_idx = call_stack[call_stack_ptr][0];
              pc = call_stack[call_stack_ptr][1];
              loadLineFromEEPROM(exec_line_idx);
          } else {
              error = true;
              error_line = exec_line_idx;
              error_msg_P = call_stack_underflow_msg;
          }
      } else {
          if (!executeCommand(pc, exec_line_idx)) {
            error = true;
            error_line = exec_line_idx;
            error_msg_P = syntax_error_msg;
          }
      }
      if (flags.stack_overflow) {
          error = true;
          error_line = exec_line_idx;
          error_msg_P = stack_overflow_msg;
          flags.running = 0;
      }
      if (error) break;
      // Use custom isspace
      bool should_print_trace_line = (flags.tracing && !my_isspace(current_char) && current_char != '!' && current_char != '\'' && current_char != '$') ||
                                     (current_char == '}' && tracing_was_on_before) ||
                                     (current_char == '{' && !tracing_was_on_before);
      if (should_print_trace_line) {
          TV.println();
          print_stack_content();
          if (current_char == '}') {
              TV.print(" (after })");
          } else if (current_char == '{') {
              TV.print(" (after {)");
          } else {
              TV.print(" (after ");
              TV.print(current_char);
              TV.print(")");
          }
      }
      if (error) break;
      if (handled_special_command) continue;
    }
    if (error) {
      break;
    }
    if (flags.running && pc >= buffer_len) {
         exec_line_idx++;
         pc = 0;
         TV.println();
    }
  }
  flags.running = 0;
  flags.tracing = 0;
  flags.stack_overflow = 0;
  fill_flag = 0; // Сброс флага заливки при завершении программы
  if (error) {
    drawCursor(false);
    TV.clear_screen();
    TV.set_cursor(0, 15 * CHAR_HEIGHT);
    TV.print("LINE ");
    TV.print(error_line);
    TV.print(" ");
    const char* msg_ptr = error_msg_P ? error_msg_P : syntax_error_msg;
    while (char c = pgm_read_byte(msg_ptr++)) {
      TV.print(c);
    }
    delay(1500);
  } else {
    TV.set_cursor(0, 12 * CHAR_HEIGHT);
    for(int i=0; i<CHARS_PER_LINE; ++i) TV.print(' ');
    TV.set_cursor(0, 12 * CHAR_HEIGHT);
    TV.print("STACK: ");
    print_stack_content();
    TV.set_cursor(0, 13 * CHAR_HEIGHT);
    for(int i=0; i<CHARS_PER_LINE; ++i) TV.print(' ');
    TV.set_cursor(0, 14 * CHAR_HEIGHT);
    for(int i=0; i<CHARS_PER_LINE; ++i) TV.print(' ');
    TV.set_cursor(0, 13 * CHAR_HEIGHT);
    const char* msg_ptr = program_ended_msg;
    while (char c = pgm_read_byte(msg_ptr++)) {
        TV.print(c);
    }
    TV.set_cursor(0, 14 * CHAR_HEIGHT);
    msg_ptr = press_any_key_msg;
    while (char c = pgm_read_byte(msg_ptr++)) {
        TV.print(c);
    }
    while (keyboard.available()) keyboard.read();
    while (!keyboard.available()) {}
    keyboard.read();
  }
  clearLineBuffer();
  cursor_x = 1;
  current_line = 0;
  cursor_y = current_line % LINES_PER_SCREEN;
  redrawAllLines(current_line);
  last_blink = millis();
  flags.cursor_visible = true;
  flags.tracing = 0;
}

void setup() {
  delay(500);
  TV.begin(VIDEO_SYSTEM, SCREEN_WIDTH, SCREEN_HEIGHT);
  TV.force_outstart(14);
  TV.select_font(FONT_TYPE);
  TV.clear_screen();
  TV.set_cursor(0, 0);
  TV.set_hbi_hook(keyboard.begin());
  initMacroEEPROM();
  // Use custom strlen_P
  uint16_t msg_len = my_strlen_P(welcome_msg);
  TV.set_cursor(((CHARS_PER_LINE - msg_len) / 2) * CHAR_WIDTH, (LINES_PER_SCREEN / 2 - 2) * CHAR_HEIGHT);
  const char* ptr = welcome_msg;
  while (char c = pgm_read_byte(ptr++)) TV.print(c);
  TV.println();
  ptr = separator;
  // Use custom strlen_P
  uint16_t sep_len = my_strlen_P(separator);
  TV.set_cursor(((CHARS_PER_LINE - sep_len) / 2) * CHAR_WIDTH, (LINES_PER_SCREEN / 2 - 1) * CHAR_HEIGHT);
  while (char c = pgm_read_byte(ptr++)) TV.print(c);
  TV.println();
  delay(2000);
  playWelcomeMelody();
  TV.clear_screen();
  current_line = 0;
  cursor_y = 0;
  loadLineFromEEPROM(current_line);
  cursor_x = buffer_len + 1;
  redrawAllLines(current_line);
}

void loop() {
  if (!flags.running) {
    handleCursorBlink();
  }
  if (keyboard.available()) {
    int c = keyboard.read();
    int current_freq = TONE_FREQ_KEY;
    int current_duration = TONE_DURATION_MS_SHORT;
    // Use custom isprint
    // Use custom toupper below
    if (flags.cursor_visible && c != PS2_LEFTARROW && c != PS2_RIGHTARROW && c != PS2_UPARROW && c != PS2_DOWNARROW && c != PS2_DELETE && c != PS2_HOME && c != PS2_END && my_isprint(c)) {
      drawCursor(false);
      flags.cursor_visible = false;
    }
    if (c == PS2_ENTER || c == '\r' || c == '\n') {
      current_freq = TONE_FREQ_ENTER;
      current_duration = TONE_DURATION_MS_LONG;
      playTone(current_freq, current_duration);
      // Use custom strcmp
      if (my_strcmp(line_buffer, "SAV") == 0) {
        saveAllLines();
      } else if (my_strcmp(line_buffer, "ERS") == 0) {
        eraseNonEmptyLines();
      } else if (my_strcmp(line_buffer, "FREERAM") == 0) {
        handleFreeRamCommand();
      } else if (my_strcmp(line_buffer, "RUN") == 0) {
        saveLineToEEPROM(current_line);
        runProgram();
      } else if (my_strcmp(line_buffer, "TEST") == 0) {
        saveLineToEEPROM(current_line);
        loadTestProgramFromFlash(test_program_flash);
        runProgram();
      } else {
        saveLineToEEPROM(current_line);
        clearLineBuffer();
        cursor_x = 1;
        current_line++;
        if (current_line >= MAX_LINES) {
          current_line = 0;
        }
        cursor_y = current_line % LINES_PER_SCREEN;
        loadLineFromEEPROM(current_line);
        // Ensure desired_cursor_x is declared here
        int desired_cursor_x = cursor_x;
        cursor_x = min(desired_cursor_x, buffer_len + 1);
        if (cursor_x < 1) cursor_x = 1;
        redrawAllLines(current_line);
      }
    } else if (c == PS2_BACKSPACE) {
      current_freq = TONE_FREQ_BACKSP;
      current_duration = TONE_DURATION_MS_LONG;
      playTone(current_freq, current_duration);
      if (cursor_x > 1 && buffer_len > 0) {
        // Corrected backspace logic: shift left from cursor position
        for (int i = cursor_x - 2; i < buffer_len - 1; i++) {
          line_buffer[i] = line_buffer[i + 1];
        }
        buffer_len--;
        line_buffer[buffer_len] = 0;
        cursor_x--;
        redrawScreenLine(cursor_y, true);
        last_blink = millis();
        flags.cursor_visible = true;
      }
    } else if (c == PS2_DELETE) {
      current_freq = TONE_FREQ_BACKSP;
      current_duration = TONE_DURATION_MS_LONG;
      playTone(current_freq, current_duration);
      if (cursor_x - 1 < buffer_len) {
        for (int i = cursor_x - 1; i < buffer_len - 1; i++) {
          line_buffer[i] = line_buffer[i + 1];
        }
        buffer_len--;
        line_buffer[buffer_len] = 0;
        redrawScreenLine(cursor_y, true);
        last_blink = millis();
        flags.cursor_visible = true;
      }
    } else if (c == PS2_LEFTARROW) {
      current_freq = TONE_FREQ_KEY;
      current_duration = TONE_DURATION_MS_SHORT;
      playTone(current_freq, current_duration);
      if (cursor_x > 1) {
        cursor_x--;
        updateCursorPosition();
        last_blink = millis();
        flags.cursor_visible = true;
      }
    } else if (c == PS2_RIGHTARROW) {
      current_freq = TONE_FREQ_KEY;
      current_duration = TONE_DURATION_MS_SHORT;
      playTone(current_freq, current_duration);
      if (cursor_x < buffer_len + 1 && cursor_x < MAX_LINE_LENGTH + 1) {
        cursor_x++;
        updateCursorPosition();
        last_blink = millis();
        flags.cursor_visible = true;
      }
    } else if (c == PS2_UPARROW) {
      current_freq = TONE_FREQ_KEY;
      current_duration = TONE_DURATION_MS_SHORT;
      playTone(current_freq, current_duration);
      saveLineToEEPROM(current_line);
      int desired_cursor_x = cursor_x; // Объявление здесь
      if (current_line == 0) {
        current_line = MAX_LINES - 1;
      } else {
        current_line--;
      }
      cursor_y = current_line % LINES_PER_SCREEN;
      loadLineFromEEPROM(current_line);
      cursor_x = min(desired_cursor_x, buffer_len + 1);
      if (cursor_x < 1) cursor_x = 1;
      redrawAllLines(current_line);
      last_blink = millis();
      flags.cursor_visible = true;
    } else if (c == PS2_DOWNARROW) {
      current_freq = TONE_FREQ_ENTER;
      current_duration = TONE_DURATION_MS_LONG;
      playTone(current_freq, current_duration);
      saveLineToEEPROM(current_line);
      int desired_cursor_x = cursor_x; // Объявление здесь
      current_line++;
      if (current_line >= MAX_LINES) {
        current_line = 0;
      }
      cursor_y = current_line % LINES_PER_SCREEN;
      loadLineFromEEPROM(current_line);
      cursor_x = min(desired_cursor_x, buffer_len + 1);
      if (cursor_x < 1) cursor_x = 1;
      redrawAllLines(current_line);
      last_blink = millis();
      flags.cursor_visible = true;
    } else if (c == PS2_HOME) {
      current_freq = TONE_FREQ_KEY;
      current_duration = TONE_DURATION_MS_SHORT;
      playTone(current_freq, current_duration);
      cursor_x = 1;
      updateCursorPosition();
      last_blink = millis();
      flags.cursor_visible = true;
    } else if (c == PS2_END) {
      current_freq = TONE_FREQ_KEY;
      current_duration = TONE_DURATION_MS_SHORT;
      playTone(current_freq, current_duration);
      cursor_x = buffer_len + 1;
      updateCursorPosition();
      last_blink = millis();
      flags.cursor_visible = true;
    } else if (my_isprint(c)) { // Use custom isprint
      if (buffer_len < MAX_LINE_LENGTH) {
        current_freq = TONE_FREQ_KEY;
        current_duration = TONE_DURATION_MS_SHORT;
        playTone(current_freq, current_duration);
        // Shift characters to the right to make space for the new character
        for (int i = buffer_len; i >= cursor_x - 1; i--) {
          line_buffer[i + 1] = line_buffer[i];
        }
        line_buffer[cursor_x - 1] = my_toupper(c); // Use custom toupper
        buffer_len++;
        cursor_x++;
        redrawScreenLine(cursor_y, true);
        last_blink = millis();
        flags.cursor_visible = true;
      } else {
        playTone(TONE_FREQ_BUFFER_FULL, TONE_DURATION_MS_SHORT);
      }
    }
  }
}

// end
