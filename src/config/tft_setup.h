#ifndef TFT_SETUP_H
#define TFT_SETUP_H

#define USER_SETUP_LOADED

#define ST7789_DRIVER

#define TFT_WIDTH  170
#define TFT_HEIGHT 320

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

#define SPI_FREQUENCY  27000000
#define SPI_READ_FREQUENCY  20000000
#define SPI_TOUCH_FREQUENCY  2500000

// DMA if needed
#define DMA_ENABLE

#endif // TFT_SETUP_H