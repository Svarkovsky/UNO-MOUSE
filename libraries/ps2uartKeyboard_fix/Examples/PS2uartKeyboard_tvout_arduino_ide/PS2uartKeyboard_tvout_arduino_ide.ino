/*
  Простой тестовый скетч для проверки работы PS/2 клавиатуры
  с библиотеками TVout и PS2uartKeyboard на Arduino Uno.

  Выводит на экран телевизора символы, набираемые на клавиатуре.

  Подключение:
  PS/2 Keyboard Data -> Arduino Digital Pin 0 (RX)
  PS/2 Keyboard Clock -> Arduino Digital Pin 4 (XCK)
  PS/2 Keyboard 5V -> Arduino 5V
  PS/2 Keyboard Ground -> Arduino GND

  TVout подключение:
  Video -> Arduino Digital Pin 7
  Sync -> Arduino Digital Pin 9
  Ground -> Arduino GND
*/

#include <TVout.h>
#include <font6x8.h> // Используем шрифт 6x8
#include <PS2uartKeyboard.h>

// Создаем объекты TVout и PS2uartKeyboard
TVout TV;
PS2uartKeyboard keyboard;

void setup() {
  // Инициализация TVout. Используем NTSC, разрешение по умолчанию (128x96)
  // или другое, если нужно. Для простого текста 128x96 достаточно.
  // Если нужно разрешение 96x64, как в Mouse, можно указать:
  // TV.begin(_NTSC, 96, 64);
  TV.begin(_NTSC); // Инициализация TVout для NTSC

  // Выбираем шрифт
  TV.select_font(font6x8);

  // Очищаем экран
  TV.clear_screen();

  // Выводим приветственное сообщение
  TV.println("Keyboard Test");
  TV.println("Type something:");

  // Устанавливаем курсор ниже приветствия
  TV.set_cursor(0, 2 * 8); // 2 строки по 8 пикселей высотой

  // Инициализация PS2uartKeyboard.
  // Важно использовать set_hbi_hook для совместимости с TVout.
  // PS2uartKeyboard::begin() возвращает указатель на функцию,
  // которую TVout будет вызывать во время горизонтального гашения.
  TV.set_hbi_hook(keyboard.begin());

  // Если ваша версия TVout не поддерживает set_hbi_hook,
  // попробуйте просто keyboard.begin(); но могут быть помехи на экране.
  // keyboard.begin(); // Альтернативная инициализация без хука
}

void loop() {
  // Проверяем, есть ли доступные символы с клавиатуры
  if (keyboard.available()) {
    // Читаем символ
    char c = keyboard.read();

    // Выводим символ на экран телевизора
    TV.print(c);

    // Обработка Backspace для стирания символа
    if (c == '\b') {
        // Перемещаем курсор назад на 1 символ
        uint8_t current_x = TV.get_cursor_x();
        uint8_t current_y = TV.get_cursor_y();
        if (current_x >= 6) { // Если не в самом начале строки
            TV.set_cursor(current_x - 6, current_y);
            TV.print(' '); // Затираем предыдущий символ пробелом
            TV.set_cursor(current_x - 6, current_y); // Возвращаем курсор
        } else if (current_y >= 8) { // Если в начале строки, переходим на предыдущую
             // Это сложнее, так как нужно знать, где закончилась предыдущая строка.
             // Для простоты, если в начале строки, Backspace ничего не делает.
             // Если нужна полная эмуляция терминала, потребуется буферизация вывода.
             // Пока оставим так.
        }
    }
     // Обработка Enter для перехода на новую строку
    if (c == '\r' || c == '\n') {
        TV.println(""); // Переход на новую строку
    }
  }
}

