#include <TVout.h>
#include <font4x6.h>
#include <PS2KeyAdvanced.h>

TVout TV;
PS2KeyAdvanced keyboard;

void setup() {
  Serial.begin(9600);
  
  TV.begin(NTSC, 120, 96);
  TV.select_font(font4x6);
  TV.clear_screen();
  
  keyboard.begin(2, 3); // Инициализация клавиатуры
  TV.println("Keyboard test");
}

void loop() {
  static uint32_t timer = millis();
  
  // Проверка клавиатуры
  if (keyboard.available()) {
    uint16_t key = keyboard.read();
    TV.print(key, HEX);
    TV.print(" ");
  }
  
  // Мигающий курсор
  if (millis() - timer > 500) {
    timer = millis();
    TV.print(".");
  }
  
  TV.delay_frame(1);
}