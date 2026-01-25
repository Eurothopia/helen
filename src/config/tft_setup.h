#ifndef TFT_SETUP_H
#define TFT_SETUP_H

#define USER_SETUP_LOADED

#ifdef WOKWI
  #define ILI9341_DRIVER
  #define TFT_WIDTH  240
  #define TFT_HEIGHT 320
#else
  #define ST7789_DRIVER
  #define TFT_WIDTH  170
  #define TFT_HEIGHT 320
#endif

#define TFT_RGB_ORDER TFT_BGR

#define TFT_MISO -1
#define TFT_MOSI 19    // SDA
#define TFT_SCLK 23    // SCL
#define TFT_CS   5     
#define TFT_DC   18    
#define TFT_RST  -1   

#define TOUCH_CS -1

#define LOAD_GLCD   
#define LOAD_FONT2  
#define LOAD_FONT4  
#define LOAD_FONT6  
#define LOAD_FONT7  
#define LOAD_FONT8  
#define LOAD_GFXFF
#define SMOOTH_FONT 

#ifdef WOKWI
  #define SPI_FREQUENCY  40000000  // Wokwi can't handle 80MHz
#else
  #define SPI_FREQUENCY  80000000
#endif

#define SPI_READ_FREQUENCY  20000000
#define SPI_TOUCH_FREQUENCY  2500000

// DMA if needed
#ifdef WOKWI
  #undef DMA_ENABLE  // Wokwi doesn't support DMA
#else
  #define DMA_ENABLE
#endif
#define DMA_ENABLE

#endif // TFT_SETUP_H