#include <TVout.h>
#include <PS2uartKeyboard.h>
#include <fontALL.h>
#include <ctype.h>

// --- Константы для TVout ---
#define VIDEO_SYSTEM NTSC // Попробуйте PAL, если NTSC не работает
#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 96
#define FONT_TYPE font4x6
#define CHAR_WIDTH 4
#define CHAR_HEIGHT 6
#define CHARS_PER_LINE (SCREEN_WIDTH / CHAR_WIDTH)
#define LINES_PER_SCREEN (SCREEN_HEIGHT / CHAR_HEIGHT)

// --- Константы для звуков ---
#define TONE_DURATION_MS_SHORT 30
#define TONE_DURATION_MS_LONG 50
#define TONE_FREQ_KEY 800
#define TONE_FREQ_BACKSP 400
#define TONE_FREQ_ENTER 1200

// --- Константы для курсора ---
#define CURSOR_BLINK_INTERVAL 500 // Установлено по вашему предпочтению

// --- Глобальные объекты ---
TVout TV;
PS2uartKeyboard keyboard;

// --- Глобальные переменные ---
int cursor_x = 0; // Позиция курсора в символах (по X)
int cursor_y = 0; // Позиция курсора в символах (по Y)
bool cursor_visible = false; // Состояние курсора (вкл/выкл)
unsigned long last_blink = 0; // Время последнего мигания
char line_buffer[CHARS_PER_LINE] = {0}; // Буфер для текущей строки
int buffer_len = 0; // Длина текста в буфере

// --- Вспомогательная функция для воспроизведения звука ---
void playTone(int freq, int duration) {
  TV.tone(freq, duration);
  delay(duration + 5);
  TV.noTone();
}

// --- Обновление позиции курсора на экране ---
void updateCursorPosition() {
  TV.set_cursor(cursor_x * CHAR_WIDTH, cursor_y * CHAR_HEIGHT);
}

// --- Отрисовка или стирание курсора ---
void drawCursor(bool show) {
  updateCursorPosition();
  if (show) {
    TV.print('_');
  } else {
    // Восстанавливаем символ из буфера или пробел
    if (cursor_x - 1 < buffer_len) {
      TV.print(line_buffer[cursor_x - 1]);
    } else {
      TV.print(' ');
    }
    delay(10); // Даем время на отрисовку
  }
}

// --- Обработка мигания курсора ---
void handleCursorBlink() {
  unsigned long current_millis = millis();
  if (current_millis - last_blink >= CURSOR_BLINK_INTERVAL) {
    cursor_visible = !cursor_visible;
    drawCursor(cursor_visible);
    last_blink = current_millis;
  }
}

// --- Обновление экрана для текущей строки ---
void redrawLine() {
  TV.set_cursor(0, cursor_y * CHAR_HEIGHT);
  TV.print(">");
  for (int i = 0; i < buffer_len && i < CHARS_PER_LINE - 1; i++) {
    TV.print(line_buffer[i]);
  }
  // Очищаем остаток строки
  for (int i = buffer_len; i < CHARS_PER_LINE - 1; i++) {
    TV.print(' ');
  }
  updateCursorPosition();
}

// --- Setup ---
void setup() {
  delay(500);

  // Инициализация TVout
  TV.begin(VIDEO_SYSTEM, SCREEN_WIDTH, SCREEN_HEIGHT);

  // --- Adjust horizontal offset ---
  // Default value can be around 10-15.
  // Try starting with 10 or 12 and increase/decrease
  // until the image is centered. 14 is a starting value.
  TV.force_outstart(14); // <-- Starting value
  // ---------------------------------------------
  
  TV.select_font(FONT_TYPE);
  TV.clear_screen();
  TV.set_cursor(0, 0);

  // Инициализация клавиатуры с привязкой к HBI hook
  TV.set_hbi_hook(keyboard.begin());

  // Вывод приветственного сообщения
  cursor_x = 0;
  cursor_y = 0;
  TV.println("TEST");
  TV.println("----");
  TV.println("");
  TV.print(">");
  cursor_x = 1; // После ">"
  cursor_y = 3; // После трех строк
  updateCursorPosition();
}

// --- Loop ---
void loop() {
  // Обработка мигания курсора
  handleCursorBlink();

  // Проверяем клавиатуру
  if (keyboard.available()) {
    char c = keyboard.read();
    int current_freq = TONE_FREQ_KEY;
    int current_duration = TONE_DURATION_MS_SHORT;
    bool handled = false;

    // Скрываем курсор перед изменением экрана, кроме стрелок
    if (cursor_visible && c != PS2_LEFTARROW && c != PS2_RIGHTARROW) {
      drawCursor(false);
      cursor_visible = false;
    }

    // --- Обработка Enter ---
    if (c == PS2_ENTER || c == '\r' || c == '\n') {
      current_freq = TONE_FREQ_ENTER;
      current_duration = TONE_DURATION_MS_LONG;
      playTone(current_freq, current_duration);

      // Очищаем буфер
      for (int i = 0; i < CHARS_PER_LINE; i++) {
        line_buffer[i] = 0;
      }
      buffer_len = 0;

      cursor_x = 0;
      cursor_y++;
      if (cursor_y >= LINES_PER_SCREEN) {
        TV.clear_screen();
        cursor_y = 0;
        cursor_x = 0;
        TV.set_cursor(0, 0);
        TV.print(">");
        cursor_x = 1;
        cursor_y = 0;
      } else {
        TV.set_cursor(0, cursor_y * CHAR_HEIGHT);
        TV.print(">");
        cursor_x = 1;
      }
      updateCursorPosition();
      handled = true;
    }
    // --- Обработка Backspace ---
    else if (c == PS2_BACKSPACE) { // PS2_BACKSPACE = 127
      current_freq = TONE_FREQ_BACKSP;
      current_duration = TONE_DURATION_MS_LONG;
      playTone(current_freq, current_duration);

      if (cursor_x > 1 && buffer_len > 0) {
        // Сдвигаем символы в буфере влево
        for (int i = cursor_x - 1; i < buffer_len - 1; i++) {
          line_buffer[i] = line_buffer[i + 1];
        }
        line_buffer[buffer_len - 1] = 0;
        buffer_len--;
        cursor_x--;
        redrawLine();
      }
      handled = true;
    }
    // --- Обработка стрелок ---
    else if (c == PS2_LEFTARROW) {
      current_freq = TONE_FREQ_KEY;
      current_duration = TONE_DURATION_MS_SHORT;
      playTone(current_freq, current_duration);
      if (cursor_x > 1) {
        cursor_x--;
        updateCursorPosition();
        redrawLine(); // Перерисовываем строку
      }
      handled = true;
    }
    else if (c == PS2_RIGHTARROW) {
      current_freq = TONE_FREQ_KEY;
      current_duration = TONE_DURATION_MS_SHORT;
      playTone(current_freq, current_duration);
      if (cursor_x < buffer_len + 1 && cursor_x < CHARS_PER_LINE - 1) {
        cursor_x++;
        updateCursorPosition();
        redrawLine(); // Перерисовываем строку
      }
      handled = true;
    }
    // --- Обработка печатаемых символов ---
    else if (isprint(c)) {
      current_freq = TONE_FREQ_KEY;
      current_duration = TONE_DURATION_MS_SHORT;
      playTone(current_freq, current_duration);

      if (buffer_len < CHARS_PER_LINE - 1) {
        // Сдвигаем символы вправо от позиции курсора
        for (int i = buffer_len; i >= cursor_x - 1; i--) {
          line_buffer[i + 1] = line_buffer[i];
        }
        line_buffer[cursor_x - 1] = toupper(c);
        buffer_len++;
        cursor_x++;
        redrawLine();
      }
      handled = true;
    }
    // --- Игнорируем остальные клавиши ---
    else {
      playTone(current_freq, current_duration);
      handled = true;
    }
  }
}