#pragma once

//FÜR ESP 32

//// SPI Pins (ESP32 Standard)
//#define TFT_SCK   18  //SCK
//#define TFT_MOSI  23  //SDA
//
//// Display Steuerpins
//#define TFT_CS    5
//#define TFT_DC    16
//#define TFT_RST   17 //RES
//#define TFT_BL    4
//
//// Rotary Encoder
//#define ENC_CLK  32
//#define ENC_DT   33
//#define ENC_SW   25
//
//// Navigation Buttons ,gegen GND -> INPUT_PULLUP
//#define BTN_LEFT    26
//#define BTN_RIGHT  27

//Für ETH01

// ST7735 (SPI)
#define TFT_SCK   14
#define TFT_MOSI  15
#define TFT_CS     4
#define TFT_DC    12
#define TFT_RST   2
// TFT_BL -> direkt an 3V3 (nicht an GPIO)

#define ENC_CLK  36
#define ENC_DT   39
#define ENC_SW   35   // gegen GND, INPUT_PULLUP

#define BTN_LEFT   5
#define BTN_RIGHT 17