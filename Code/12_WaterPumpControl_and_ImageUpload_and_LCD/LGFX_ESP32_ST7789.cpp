/**
 * LGFX_ESP32_ST7789.cpp
 *
 * This file provides helper functions for controlling an LCD display using the LovyanGFX library on ESP32 hardware.
 * Designed for use with LGFX_ESP32_ST7789.hpp, which configures the ST7789 LCD panel.
 *
 * Features:
 * - LCD initialization and rotation
 * - Sprite creation and font management
 * - Custom background and formatted text rendering
 * - Efficient SPI transaction handling
 *
 * Usage:
 *   #include "LGFX_ESP32_ST7789.hpp"
 *   lcdInit(); // Initialize LCD
 *   spriteSetFont(&fonts::Font0); // Set font
 *   spriteDrawBackground(); // Draw background
 *   spritePrintf(10, 30, 0xFFFF00, "Moisture: %d%%", moistureValue); // Print text
 *
 * Hardware: ESP32 + ST7789 LCD
 * Library: LovyanGFX (https://github.com/lovyan03/LovyanGFX)
 * Author: John Leung
 * Date: February 26, 2026
 */
#include "LGFX_ESP32_ST7789.hpp"

LGFX_Custom lcd;
LGFX_Sprite sprite(&lcd);

/**
 * API (Application Programming Interface)
 * These functions are defined in LGFX_ESP32_ST7789.cpp and can be called from the main sketch (lovyangfx_moisture_printf.ino) to interact with the display and sprite.
 */
void lcdInit() {
    lcd.init();
    lcd.setRotation(1);
}

/**
 * A helper function to set the font for the sprite. It takes a pointer to an IFont object and applies it to the sprite. This allows for flexible font management when drawing text on the sprite.
 * @param font A pointer to an IFont object that defines the font to be used for drawing text on the sprite.
 * This function is called in the setup to set the initial font and can be called in the loop to change fonts dynamically before drawing text.
 */
void spriteSetFont(const lgfx::IFont* font) {
  lcd.startWrite(); // Start SPI transaction
  sprite.setFont(font);
  lcd.endWrite();   // End SPI transaction
}

/**
 * A helper function to draw the background pattern on the sprite. It fills the sprite with a gradient pattern based on pixel coordinates.
 * This function is called in the loop to clear the previous text before drawing new text.
 */
void spriteDrawBackground() {
  lcd.startWrite();
  sprite.createSprite(lcd.width(), lcd.height());
  for (int y = 0; y < sprite.height(); ++y) {
    for (int x = 0; x < sprite.width(); ++x) {
      sprite.drawPixel(x, y, lcd.color888(x>>1, (x + y) >> 2, y>>1));
    }
  }
  lcd.endWrite();
}

/**
 * A helper function to print formatted text on the sprite. It takes coordinates, text color, and a format string with variable arguments to display dynamic content.
 * @param x The x-coordinate on the sprite where the text will start.
 * @param y The y-coordinate on the sprite where the text will start.
 * @param textcolor The color of the text to be drawn, specified as a 24
 * -bit RGB value (e.g., 0xFFFF00 for yellow).
 * @param format A C-style format string that specifies how to format the text, similar to printf in C/C++. It can include format specifiers like %d for integers, %s for strings, etc.
 * This function uses variable argument lists (va_list) to handle the format string and its arguments, allowing for flexible and dynamic text rendering on the sprite. After drawing the text, it pushes the sprite to the display at coordinates (0, 0).
 */
void spritePrintf(int32_t x, int32_t y, uint32_t textcolor, const char * __restrict format, ...)
{
  lcd.startWrite();
  sprite.setTextColor(textcolor);
  sprite.setCursor(x, y);
  
  va_list args;
  va_start(args, format);
  sprite.vprintf(format, args);
  va_end(args);

  sprite.pushSprite(0, 0);
  lcd.endWrite();
}