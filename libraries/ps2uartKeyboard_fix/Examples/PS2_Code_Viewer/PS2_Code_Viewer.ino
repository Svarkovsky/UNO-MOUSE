/*
  Простая программа для просмотра кодов клавиш PS/2 клавиатуры
  с библиотеками TVout и PS2uartKeyboard на Arduino Uno.

  Выводит на экран телевизора десятичные и шестнадцатеричные коды
  нажатых клавиш.

  Подключение:
  PS/2 Keyboard Data -> Arduino Digital Pin 0 (RX)
  PS/2 Keyboard Clock -> Arduino Digital Pin 4 (XCK)
  PS/2 Keyboard 5V -> Arduino 5V
  PS/2 Keyboard Ground -> Arduino GND

  TVout подключение:
  Video -> Arduino Digital Pin 7
  Sync -> Arduino Digital Pin 9
  Ground -> Arduino GND
*//*
  Простая программа для просмотра кодов клавиш PS/2 клавиатуры
  с библиотеками TVout и PS2uartKeyboard на Arduino Uno.

  Выводит на экран телевизора десятичные и шестнадцатеричные коды
  нажатых клавиш.

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
#include <font4x6.h> // Используем шрифт 
#include <PS2uartKeyboard.h> // Используем вашу модифицированную библиотеку
#include <ctype.h>           // Для функции isprint
#include <stdlib.h>          // Для функции itoa (преобразование числа в строку)

// Создаем объекты TVout и PS2uartKeyboard
TVout TV;
PS2uartKeyboard keyboard;

// Буфер для преобразования числовых кодов в строки для вывода
char code_buffer[10]; // Достаточно для числа до 255 и его шестнадцатеричного представления

// Отслеживаем текущую строку вывода на экране (в символьных строках)
int current_screen_line = 0;

// Определяем высоту шрифта и экрана в символьных строках
// Используем константу из файла шрифта font4x6.h
#define FONT_HEIGHT_PIXELS 6 // Высота шрифта в пикселях
// Высота экрана для NTSC по умолчанию 96 пикселей
#define SCREEN_HEIGHT_PIXELS 96
#define TOTAL_SCREEN_CHAR_LINES (SCREEN_HEIGHT_PIXELS / FONT_HEIGHT_PIXELS) // 96 / 8 = 12 строк

void setup() {
  // Инициализация TVout для NTSC. Разрешение по умолчанию 128x96.
  TV.begin(_NTSC);

  // Выбираем шрифт
  TV.select_font(font4x6);

  // Очищаем экран
  TV.clear_screen();

  // Выводим приветственное сообщение
  TV.println("PS/2 Code Viewer");
  TV.println("Press keys...");

  // Устанавливаем курсор ниже приветствия для вывода кодов
  TV.set_cursor(0, 2 * FONT_HEIGHT_PIXELS); // 2 строки заголовка * высота шрифта
  current_screen_line = 2; // Начинаем вывод кодов с 3-й строки экрана (индекс 2)

  // Инициализация PS2uartKeyboard.
  // Важно использовать set_hbi_hook для совместимости с TVout.
  // PS2uartKeyboard::begin() возвращает указатель на функцию,
  // которую TVout будет вызывать во время горизонтального гашения.
  TV.set_hbi_hook(keyboard.begin());
}

void loop() {
  // Проверяем, есть ли доступные символы/коды с клавиатуры
  if (keyboard.available()) {
    // Читаем символ/код
    int c = keyboard.read();

    // Проверяем, не достигли ли мы конца экрана для вывода кодов.
    // Если текущая строка вывода + 1 (для следующего println) >= общее количество строк на экране
    if (current_screen_line + 1 >= TOTAL_SCREEN_CHAR_LINES) {
         TV.clear_screen();
         TV.set_cursor(0, 0);
         TV.println("PS/2 Code Viewer"); // Снова выводим заголовок
         TV.println("Press keys...");
         TV.set_cursor(0, 2 * FONT_HEIGHT_PIXELS); // Снова устанавливаем курсор ниже заголовка
         current_screen_line = 2; // Сбрасываем счетчик строк
    }

    // Устанавливаем курсор на текущую строку вывода
    TV.set_cursor(0, current_screen_line * FONT_HEIGHT_PIXELS);

    // Выводим метку "Code: "
    TV.print("Code: ");

    // Выводим десятичный код
    itoa(c, code_buffer, 10); // Преобразуем число c в строку в десятичной системе
    TV.print(code_buffer);

    // Выводим шестнадцатеричный код
    TV.print(" (0x");
    itoa(c, code_buffer, 16); // Преобразуем число c в строку в шестнадцатеричной системе
    TV.print(code_buffer);
    TV.print(")");

    // Если код соответствует печатаемому символу, выводим его тоже
    if (isprint(c)) {
      TV.print(" Char: '");
      TV.print((char)c); // Выводим сам символ
      TV.print("'");
    }

    // Переходим на новую строку для следующего кода
    // TV.println() автоматически переводит курсор на следующую строку.
    TV.println();
    current_screen_line++; // Увеличиваем счетчик строк после вывода

  }

  // В loop() не нужна явная задержка, так как операции TVout и keyboard.available()
  // уже занимают достаточно времени и обеспечивают стабильность.
}

