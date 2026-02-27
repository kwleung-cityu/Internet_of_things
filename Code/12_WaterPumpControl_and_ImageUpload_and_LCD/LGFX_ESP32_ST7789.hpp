#pragma once

#define LGFX_USE_V1
#include <LGFX_AUTODETECT.hpp>
#include <LovyanGFX.hpp>

class LGFX_Custom : public lgfx::LGFX_Device
{
  lgfx::Panel_ST7789  _panel_instance;
  lgfx::Bus_SPI       _bus_instance;   // SPIバスのインスタンス
  lgfx::Light_PWM     _light_instance;

public:

  LGFX_Custom(void)
  {
    { 
      auto cfg = _bus_instance.config();    // Get structure for bus configuration

      cfg.spi_host = SPI2_HOST;     // Select SPI to use  ESP32-S2,C3 : SPI2_HOST or SPI3_HOST / ESP32 : VSPI_HOST or HSPI_HOST
      cfg.spi_mode = 0;             // Set SPI communication mode (0 ~ 3)
      cfg.freq_write = 40000000;    // SPI clock for writing (max 80MHz, rounded to integer division of 80MHz)
      cfg.freq_read  = 16000000;    // SPI clock for reading
      cfg.spi_3wire  = true;        // Set true if receiving on MOSI pin
      cfg.use_lock   = true;        // Set true to use transaction lock
      cfg.dma_channel = SPI_DMA_CH_AUTO; // Set DMA channel to use (0=No DMA / 1=1ch / 2=ch / SPI_DMA_CH_AUTO=auto)
      cfg.pin_sclk = 46;            // Set SPI SCLK pin number
      cfg.pin_mosi = 3;            // Set SPI MOSI pin number
      cfg.pin_miso = -1;            // Set SPI MISO pin number (-1 = disable)
      cfg.pin_dc   = 2;             // Set SPI D/C pin number  (-1 = disable)
      _bus_instance.config(cfg);    // Apply settings to bus
      _panel_instance.setBus(&_bus_instance);      // Set bus to panel
    }

    {
      auto cfg = _panel_instance.config();    // Get structure for panel configuration

      cfg.pin_cs           =    14;  // Pin number for CS connection   (-1 = disable)
      cfg.pin_rst          =    48;  // Pin number for RST connection  (-1 = disable)
      cfg.pin_busy         =    -1;  // Pin number for BUSY connection (-1 = disable)
      // The following values are general defaults for each panel. If unsure, try commenting out unknown items.
      cfg.panel_width      =   172;  // Actual displayable width
      cfg.panel_height     =   320;  // Actual displayable height
      cfg.offset_x         =    34;  // X offset of the panel
      cfg.offset_y         =     0;  // Y offset of the panel
      cfg.offset_rotation  =     0;  // Offset value for rotation direction 0~7 (4~7 are upside down)
      cfg.dummy_read_pixel =     8;  // Number of dummy reads before pixel read
      cfg.dummy_read_bits  =     1;  // Number of dummy reads before non-pixel data read
      cfg.readable         =  true;  // Set true if data can be read
      cfg.invert           =  true;  // Set true if panel brightness is inverted
      cfg.rgb_order        = false;  // Set true if red and blue are swapped
      cfg.dlen_16bit       = false;  // Set true if panel sends data in 16-bit units for 16bit parallel or SPI
      cfg.bus_shared       =  true;  // Set true if SD card shares the bus (bus control for drawJpgFile, etc.)

      _panel_instance.config(cfg);
    }
      //_panel_instance.setColorDepth(lgfx::v1::color_depth_t::rgb888_3Byte); 
      setPanel(&_panel_instance); // 使用するパネルをセットします。
  }
};

// API (Application Programming Interface)
void lcdInit();
void spriteDrawBackground();
void spriteSetFont(const lgfx::IFont* font);
void spritePrintf(int32_t x, int32_t y, uint32_t textcolor, const char * __restrict format, ...);