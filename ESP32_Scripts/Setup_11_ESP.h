#define ST7735_DRIVER

#define TFT_MISO -1
#define TFT_MOSI 23  // SDA
#define TFT_SCLK 18  // SCK
#define TFT_CS   17  // Chip select control pin
#define TFT_DC    5  // Data Command control pin
#define TFT_RST  16  // Reset pin
#define TFT_BL    4  // LED back-light

// For 1.77" display
#define TFT_WIDTH  128
#define TFT_HEIGHT 160

#define LOAD_GLCD   // Font 1. Original Adafruit 8 pixel font needs ~1820 bytes in FLASH
#define LOAD_FONT2  // Font 2. Small 16 pixel high font, needs ~3534 bytes in FLASH
#define LOAD_FONT4  // Font 4. Medium 26 pixel high font, needs ~5848 bytes in FLASH
#define LOAD_FONT6  // Font 6. Large 48 pixel font, needs ~2666 bytes in FLASH
#define LOAD_FONT7  // Font 7. 7 segment 48 pixel font, needs ~2438 bytes in FLASH
#define LOAD_FONT8  // Font 8. Large 75 pixel font needs ~3256 bytes in FLASH
#define LOAD_GFXFF  // FreeFonts. Include access to the 48 Adafruit_GFX free fonts FF1 to FF48

#define SMOOTH_FONT

#define SPI_FREQUENCY  27000000
#define SPI_READ_FREQUENCY  20000000
#define SPI_TOUCH_FREQUENCY  2500000

// #define SUPPORT_TRANSACTIONS