#include <TVout.h>
#include <font4x6.h>

TVout TV;

void setup() {
  Serial.begin(9600);
  
  // Инициализация TVout с базовыми параметрами
  if (TV.begin(NTSC, 120, 96)) {
    Serial.println("TVout init failed!");
    while(1); // Зависаем при ошибке
  }
  
  TV.select_font(font4x6);
  TV.clear_screen();
  TV.println("TV Test OK");
  Serial.println("TV test running");
}

void loop() {
  // Просто выводим текст и мигаем курсором
  static uint32_t timer = millis();
  if (millis() - timer > 500) {
    timer = millis();
    TV.print(".");
  }
  TV.delay_frame(1);
}