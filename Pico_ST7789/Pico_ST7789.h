// Fast ST7789 IPS 240x240 SPI display library
// (c) 2019-20 by Pawel A. Hernik
// Modified by C.Schwick

#ifndef _ST7789_FAST_H_
#define _ST7789_FAST_H_

#include <stdint.h>
#include <string.h>


// It makes sense to overwrite these in the cmake file
#define ST7789_TFTWIDTH   240
#define ST7789_TFTHEIGHT  320

#define RGBto565(r,g,b) ((((r) & 0xF8) << 8) | (((g) & 0xFC) << 3) | ((b) >> 3)) 
#define RGBIto565(r,g,b,i) ((((((r)*(i))/255) & 0xF8) << 8) | ((((g)*(i)/255) & 0xFC) << 3) | ((((b)*(i)/255) & 0xFC) >> 3)) 

// Color definitions
#define	BLACK   0x0000
#define	BLUE    0x001F
#define	RED     0xF800
#define	GREEN   0x07E0
#define CYAN    0x07FF
#define MAGENTA 0xF81F
#define YELLOW  0xFFE0
#define WHITE   0xFFFF
#define	GREY    RGBto565(128,128,128)
#define	LGREY   RGBto565(160,160,160)
#define	DGREY   RGBto565( 80, 80, 80)
#define	LBLUE   RGBto565(100,100,255)
#define	DBLUE   RGBto565(  0,  0,128)

#define LCD_ASCII_OFFSET 0x20 //0x20, ASCII character for Space, The font table starts with this character

/// Font data stored PER GLYPH
typedef struct {
  uint16_t bitmapOffset; ///< Pointer into GFXfont->bitmap
  uint8_t  width;         ///< Bitmap dimensions in pixels
  uint8_t  height;        ///< Bitmap dimensions in pixels
  uint8_t  xAdvance;      ///< Distance to advance cursor (x axis)
  int8_t   xOffset;        ///< X dist from cursor pos to UL corner
  int8_t   yOffset;        ///< Y dist from cursor pos to UL corner
} GFXglyph;

/// Data stored for FONT AS A WHOLE
typedef struct {
  uint8_t    *bitmap;     ///< Glyph bitmaps, concatenated
  GFXglyph   *glyph;     ///< Glyph array
  uint16_t    first;      ///< ASCII extents (first char)
  uint16_t    last;       ///< ASCII extents (last char)
  uint8_t     yAdvance;    ///< Newline distance (y axis)
  const char *subset;  ///< subset of chars in the font
} GFXfont;


/////////////////////////////////////////////////////////////////////

class Pico_ST7789  {

 public:
  Pico_ST7789( uint16_t width, uint16_t height, bool inversion, uint8_t rotation);

  void init();
  void setAddrWindow(uint16_t xs, uint16_t ys, uint16_t xe, uint16_t ye);
  void pushColor(uint16_t color);
  void fillScreen(uint16_t color=BLACK);
  void clearScreen() { fillScreen(BLACK); }
  void drawPixel(int16_t x, int16_t y, uint16_t color);
  void drawFastVLine(int16_t x, int16_t y, int16_t h, uint16_t color);
  void drawFastHLine(int16_t x, int16_t y, int16_t w, uint16_t color);
  void fillRect(int16_t x, int16_t y, int16_t w, int16_t h, uint16_t color);
  void drawImage(int16_t x, int16_t y, int16_t w, int16_t h, uint16_t *img);
  void drawImageF(int16_t x, int16_t y, int16_t w, int16_t h, const uint16_t *img16);
  void drawImageF(int16_t x, int16_t y, const uint16_t *img16)
  {
    drawImageF(x,y,*img16,*(img16+1),img16+3);
  } 
  void setRotation(uint8_t r);
  void invertDisplay(bool mode);
  void partialDisplay(bool mode);
  void sleepDisplay(bool mode);
  void enableDisplay(bool mode);
  void idleDisplay(bool mode);
  void resetDisplay();
  void setScrollArea(uint16_t tfa, uint16_t bfa);
  void setScroll(uint16_t vsp);
  void setPartArea(uint16_t sr, uint16_t er);
  void setBrightness(uint8_t br);
  void powerSave(uint8_t mode);

  uint16_t Color565(uint8_t r, uint8_t g, uint8_t b);
  uint16_t color565(uint8_t r, uint8_t g, uint8_t b) { return Color565(r, g, b); } 
  void rgbWheel(int idx, uint8_t *_r, uint8_t *_g, uint8_t *_b);
  uint16_t rgbWheel(int idx);

  void drawCircle(int16_t x0, int16_t y0, int16_t r, uint16_t color);
  void drawRectWH(uint8_t x, uint8_t y, uint8_t w, uint8_t h, uint16_t color);

  // Text
  void setFont(const GFXfont *f);
  void setTextWrap(bool w);
  void drawChar(uint16_t x, uint16_t y, uint8_t c, uint16_t color, uint16_t bg,  uint8_t size);
  void drawCharG(uint16_t x, uint16_t y, uint8_t c, uint16_t color, uint16_t bg,  uint8_t size);
  void drawText(uint16_t x, uint16_t y, const char *_text, uint16_t color, uint16_t bg, uint8_t size);
  void drawTextG(uint16_t x, uint16_t y, const char *_text, uint16_t color, uint16_t bg, uint8_t size);
  
protected:
  uint8_t _colstart, _rowstart, _xstart, _ystart,_rotation;
  uint16_t _width, _height;
  bool _inversion;
  void commonST7789Init(const uint8_t *cmdList);
  void displayInit(const uint8_t *addr);
  void writeSPI(uint8_t);
  void writeMulti(uint16_t color, uint32_t num);
  void copyMulti(uint8_t *img, uint16_t num);
  void writeCmd(uint8_t c);
  void writeData(uint8_t d8);
  void writeData16(uint16_t d16);
  GFXfont *_gfxFont;
  bool _wrap;


};

#endif
