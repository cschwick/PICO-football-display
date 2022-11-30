// Fast ST7789 IPS 240x240 SPI display library
// (c) 2019-20 by Pawel A. Hernik
// Modified by C. Schwick

#include "Pico_ST7789.h"
#include "hardware/spi.h"
#include "hardware/gpio.h"
#include "stdio.h"
#include "TextFonts.h"


// -----------------------------------------
// ST7789 commands

#define ST7789_NOP     0x00
#define ST7789_SWRESET 0x01

#define ST7789_SLPIN   0x10  // sleep on
#define ST7789_SLPOUT  0x11  // sleep off
#define ST7789_PTLON   0x12  // partial on
#define ST7789_NORON   0x13  // partial off
#define ST7789_INVOFF  0x20  // invert off
#define ST7789_INVON   0x21  // invert on
#define ST7789_DISPOFF 0x28  // display off
#define ST7789_DISPON  0x29  // display on
#define ST7789_IDMOFF  0x38  // idle off
#define ST7789_IDMON   0x39  // idle on

#define ST7789_CASET   0x2A
#define ST7789_RASET   0x2B
#define ST7789_RAMWR   0x2C
#define ST7789_RAMRD   0x2E

#define ST7789_COLMOD  0x3A
#define ST7789_MADCTL  0x36

#define ST7789_PTLAR    0x30   // partial start/end
#define ST7789_VSCRDEF  0x33   // SETSCROLLAREA
#define ST7789_VSCRSADD 0x37

#define ST7789_WRDISBV  0x51
#define ST7789_WRCTRLD  0x53
#define ST7789_WRCACE   0x55
#define ST7789_WRCABCMB 0x5e

#define ST7789_POWSAVE    0xbc
#define ST7789_DLPOFFSAVE 0xbd

// bits in MADCTL
#define ST7789_MADCTL_MY  0x80
#define ST7789_MADCTL_MX  0x40
#define ST7789_MADCTL_MV  0x20
#define ST7789_MADCTL_ML  0x10
#define ST7789_MADCTL_RGB 0x00

#define ST7789_240x240_XSTART 0
#define ST7789_240x240_YSTART 0

#define ST_CMD_DELAY   0x80

spi_inst_t *spi_inst;

// Initialization commands for ST7789 240x240 1.3" IPS
// taken from Adafruit

static const uint8_t init_cmds[] = {
    9,                       				      // 9 commands in list:
    ST7789_SWRESET,   ST_CMD_DELAY,  		              // 1: Software reset, no args, w/delay
      150,                     				      // 150 ms delay
    ST7789_SLPOUT ,   ST_CMD_DELAY,  		              // 2: Out of sleep mode, no args, w/delay
      255,                    				      // 255 = 500 ms delay
    ST7789_COLMOD , 1+ST_CMD_DELAY,  		              // 3: Set color mode, 1 arg + delay:
      0x55,                   				      // 16-bit color 16bit/pix 65k colors
      10,                     				      // 10 ms delay
    ST7789_MADCTL , 1,  			      	      // 4: Memory access ctrl (directions), 1 arg:
      0x00,                   				      // Row addr/col addr, bottom to top refresh
//    ST7789_CASET  , 4,  		       		      // 5: Column addr set, 4 args, no delay:
//      0x00, ST7789_240x240_XSTART,                            // XSTART = 0
//	    (ST7789_TFTWIDTH+ST7789_240x240_XSTART) >> 8,
//	    (ST7789_TFTWIDTH+ST7789_240x240_XSTART) & 0xFF,   // XEND = 240
//    ST7789_RASET  , 4,  			              // 6: Row addr set, 4 args, no delay:
//      0x00, ST7789_240x240_YSTART,                            // YSTART = 0
//      (ST7789_TFTHEIGHT+ST7789_240x240_YSTART) >> 8,
//	    (ST7789_TFTHEIGHT+ST7789_240x240_YSTART) & 0xFF,  // YEND = 240
//    ST7789_INVON ,    ST_CMD_DELAY,  		              // 7: Inversion ON
//    10,                                                       // no idea why, but needed...
    ST7789_NORON  ,   ST_CMD_DELAY,  		              // 8: Normal display on, no args, w/delay
      10,                     				      // 10 ms delay
    ST7789_DISPON ,   ST_CMD_DELAY,  		              // 9: Main screen turn on, no args, w/delay
      20
};
// -----------------------------------------

#define SPI_START
#define SPI_END

// macros for fast DC and CS state changes
#define DC_DATA     gpio_put(TFT_DC, 1);
#define DC_COMMAND  gpio_put(TFT_DC, 0);

// CS always connected to the ground: don't do anything for better performance
#define CS_IDLE
#define CS_ACTIVE


inline void Pico_ST7789::writeSPI(uint8_t data) {
    uint8_t res = spi_write_blocking(spi_inst, &data, 1);
}

// ----------------------------------------------------------
// fast method to send a fixed 16-bit values multiple times via SPI
inline void Pico_ST7789::writeMulti(uint16_t color, uint32_t num)
{
  uint8_t buf[2] = {uint8_t(color>>8),uint8_t(color & 0xff)};
  while(num--) {
    spi_write_blocking(spi_inst, buf, 2);
  }
} 

// ----------------------------------------------------------
// fast method to send multiple 16-bit values from RAM via SPI
inline void Pico_ST7789::copyMulti(uint8_t *img, uint16_t num)
{
  while(num--)
    {
      spi_write_blocking( spi_inst, (img+1), 1);
      spi_write_blocking( spi_inst, (img+0), 1);
      img+=2;
    }
  sleep_ms(10);
} 

// ----------------------------------------------------------
Pico_ST7789::Pico_ST7789( uint16_t width, uint16_t height, bool inversion, uint8_t rotation) 
{
  _width = width;
  _height = height;
  _inversion = inversion;
  _rotation = rotation;
}


// ----------------------------------------------------------
void Pico_ST7789::init() 
{
  commonST7789Init(NULL);

  if((_width==240) && (_height==240)) {
    _colstart = 0;
    _rowstart = 80;
  } else {
    _colstart = 0;
    _rowstart = 0;
  }
  _ystart = _xstart = 0;

  displayInit(init_cmds);
  if ( _inversion )
    invertDisplay( true );
  else
    invertDisplay( false );

  setRotation(_rotation);

}


// ----------------------------------------------------------
void Pico_ST7789::writeCmd(uint8_t c) 
{
  DC_COMMAND;
  CS_ACTIVE;
  SPI_START;

  writeSPI(c);

  CS_IDLE;
  SPI_END;
}

// ----------------------------------------------------------
void Pico_ST7789::writeData(uint8_t d8) 
{
  DC_DATA;
  CS_ACTIVE;
  SPI_START;
    
  writeSPI(d8);

  CS_IDLE;
  SPI_END;
}

// ----------------------------------------------------------
void Pico_ST7789::writeData16(uint16_t d16) 
{
  DC_DATA;
  CS_ACTIVE;
  SPI_START;
    
  writeMulti(d16,1);

  CS_IDLE;
  SPI_END;
}

// ----------------------------------------------------------
void Pico_ST7789::displayInit(const uint8_t *addr) 
{
  uint8_t  numCommands, numArgs;
  uint16_t ms;
  numCommands = *addr++;
    
  while(numCommands--) {
    writeCmd(*addr++);
    numArgs  = *addr++;
    ms       = numArgs & ST_CMD_DELAY;
    numArgs &= ~ST_CMD_DELAY;
    while(numArgs--) writeData(*addr++);

    if(ms) {
      ms = *addr++;
      if(ms==255) ms=500;
      sleep_ms(ms);
    }
  }
}

// ----------------------------------------------------------
// Initialization code common to all ST7789 displays
void Pico_ST7789::commonST7789Init(const uint8_t *cmdList) 
{ 
  //setup the gpios for reset and data/command

  CS_ACTIVE;
  // Define reset pin and apply a reset
  gpio_init(TFT_RST);
  gpio_set_dir(TFT_RST, GPIO_OUT);
  gpio_put(TFT_RST, 1);

  // Define data/command pin
  gpio_init(TFT_DC);
  gpio_set_dir(TFT_DC, GPIO_OUT);


  // set up the SPI: spi0 or spi1
  spi_inst = SPI_TFT_PORT;
  // This example will use SPI0 at 4.0MHz.
  // This has to be before setting up pins and format!
  //spi_init(spi_inst, 16000000); // with 65MHz was still working...
  // for scope measurents
  spi_init(spi_inst, 32000000); // with 65MHz was still working...
  // Now set up the SPI pins
  gpio_set_function(SPI_TFT_SCK, GPIO_FUNC_SPI);
  gpio_set_function(SPI_TFT_TX, GPIO_FUNC_SPI);
  gpio_set_function(SPI_TFT_RX, GPIO_FUNC_SPI);
  // The Aliexpress 240x240 wants this...:
  spi_set_format( spi_inst, 8, SPI_CPOL_1, SPI_CPHA_0, SPI_MSB_FIRST );

  sleep_ms(100);
  // apply a reset
  gpio_put(TFT_RST, 1);
  sleep_ms(50);
  gpio_put(TFT_RST,0);
  sleep_ms(50);
  gpio_put(TFT_RST,1);
  sleep_ms(50);

  // initialisation command sequence
  if(cmdList) displayInit(cmdList);
}

// ----------------------------------------------------------
void Pico_ST7789::setRotation(uint8_t m) 
{
  writeCmd(ST7789_MADCTL);
  uint8_t rotation = m & 3;
  uint16_t tmp;
  switch (rotation) {
   case 0:
     writeData(ST7789_MADCTL_MX | ST7789_MADCTL_MY | ST7789_MADCTL_RGB);
     _xstart = _colstart;
     _ystart = _rowstart;
     break;
   case 1:
     writeData(ST7789_MADCTL_MY | ST7789_MADCTL_MV | ST7789_MADCTL_RGB);
     _ystart = _colstart;
     _xstart = _rowstart;
     tmp = _width;
     _width = _height;
     _height = tmp;
     break;
  case 2:
     writeData(ST7789_MADCTL_RGB);
     _xstart = _colstart;
     //     _ystart = _rowstart;
     _ystart = 0;
     break;
   case 3:
     writeData(ST7789_MADCTL_MX | ST7789_MADCTL_MV | ST7789_MADCTL_RGB);
     _ystart = _colstart;
     _xstart = 0;
     tmp = _width;
     _width = _height;
     _height = tmp;
     break;
  }
}

// ----------------------------------------------------------
void Pico_ST7789::setAddrWindow(uint16_t xs, uint16_t ys, uint16_t xe, uint16_t ye)
{
  xs+=_xstart; xe+=_xstart;
  ys+=_ystart; ye+=_ystart;

  CS_ACTIVE;
  SPI_START;
  
  DC_COMMAND; writeSPI(ST7789_CASET);
  DC_DATA;
  writeSPI(xs >> 8); writeSPI(xs);
  writeSPI(xe >> 8); writeSPI(xe);

  DC_COMMAND; writeSPI(ST7789_RASET);
  DC_DATA;
  writeSPI(ys >> 8); writeSPI(ys);
  writeSPI(ye >> 8); writeSPI(ye);

  DC_COMMAND; writeSPI(ST7789_RAMWR);

  DC_DATA;

}

// ----------------------------------------------------------
void Pico_ST7789::pushColor(uint16_t color) 
{
  SPI_START;
  CS_ACTIVE;

  writeSPI(color>>8); writeSPI(color);

  CS_IDLE;
  SPI_END;
}

// ----------------------------------------------------------
void Pico_ST7789::drawPixel(int16_t x, int16_t y, uint16_t color) 
{
  if(x<0 ||x>=_width || y<0 || y>=_height) return;
  //setAddrWindow(x,y,x+1,y+1);
  setAddrWindow(x,y,x,y);
  writeSPI(color>>8); writeSPI(color);
  CS_IDLE;
  SPI_END;
}

// ----------------------------------------------------------
void Pico_ST7789::drawFastVLine(int16_t x, int16_t y, int16_t h, uint16_t color) 
{
  if(x>=_width || y>=_height || h<=0) return;
  if(y+h-1>=_height) h=_height-y;
  setAddrWindow(x, y, x, y+h-1);

  writeMulti(color,h);

  CS_IDLE;
  SPI_END;
}

// ----------------------------------------------------------
void Pico_ST7789::drawFastHLine(int16_t x, int16_t y, int16_t w,  uint16_t color) 
{
  if(x>=_width || y>=_height || w<=0) return;
  if(x+w-1>=_width)  w=_width-x;
  setAddrWindow(x, y, x+w-1, y);

  writeMulti(color,w);

  CS_IDLE;
  SPI_END;
}

// ----------------------------------------------------------
void Pico_ST7789::fillScreen(uint16_t color) 
{
  fillRect(0, 0,  _width, _height, color);
}

// ----------------------------------------------------------
void Pico_ST7789::fillRect(int16_t x, int16_t y, int16_t w, int16_t h, uint16_t color) 
{
  if(x>=_width || y>=_height || w<=0 || h<=0) return;
  if(x+w-1>=_width)  w=_width -x;
  if(y+h-1>=_height) h=_height-y;
  setAddrWindow(x, y, x+w-1, y+h-1);
  
  writeMulti(color,w*h);
	
  CS_IDLE;
  SPI_END;
}

// ----------------------------------------------------------
// draws image from RAM and scale
void Pico_ST7789::drawImage(int16_t x, int16_t y, int16_t w, int16_t h, uint16_t *img16) 
{
  // all protections should be on the application side
  if(w<=0 || h<=0) return;  // left for compatibility
  setAddrWindow(x, y, x+w-1, y+h-1);

  copyMulti((uint8_t *)img16, w*h);

  CS_IDLE;
  SPI_END;
}

// ----------------------------------------------------------
// draws image from flash (PROGMEM)
void Pico_ST7789::drawImageF(int16_t x, int16_t y, int16_t w, int16_t h, const uint16_t *img16) 
{
  if(x>=_width || y>=_height || w<=0 || h<=0) return;
  setAddrWindow(x, y, x+w-1, y+h-1);

  uint32_t num = (uint32_t)w*h;
  uint16_t num16 = num>>3;
  uint8_t *img = (uint8_t *)img16;
  while(num16--) {
    writeSPI(*(img+1)); writeSPI(*(img+0)); img+=2;
    writeSPI(*(img+1)); writeSPI(*(img+0)); img+=2;
    writeSPI(*(img+1)); writeSPI(*(img+0)); img+=2;
    writeSPI(*(img+1)); writeSPI(*(img+0)); img+=2;
    writeSPI(*(img+1)); writeSPI(*(img+0)); img+=2;
    writeSPI(*(img+1)); writeSPI(*(img+0)); img+=2;
    writeSPI(*(img+1)); writeSPI(*(img+0)); img+=2;
    writeSPI(*(img+1)); writeSPI(*(img+0)); img+=2;
  }
  uint8_t num8 = num & 0x7;
  while(num8--) { writeSPI(*(img+1)); writeSPI(*(img+0)); img+=2; }

  CS_IDLE;
  SPI_END;
}

// ----------------------------------------------------------
// Pass 8-bit (each) R,G,B, get back 16-bit packed color
uint16_t Pico_ST7789::Color565(uint8_t r, uint8_t g, uint8_t b) 
{
  return ((r & 0xF8) << 8) | ((g & 0xFC) << 3) | (b >> 3);
}

// ----------------------------------------------------------
void Pico_ST7789::invertDisplay(bool mode) 
{
  writeCmd(!mode ? ST7789_INVON : ST7789_INVOFF);  // modes inverted?
}

// ----------------------------------------------------------
void Pico_ST7789::partialDisplay(bool mode) 
{
  writeCmd(mode ? ST7789_PTLON : ST7789_NORON);
}

// ----------------------------------------------------------
void Pico_ST7789::sleepDisplay(bool mode) 
{
  writeCmd(mode ? ST7789_SLPIN : ST7789_SLPOUT);
  sleep_ms(5);
}

// ----------------------------------------------------------
void Pico_ST7789::enableDisplay(bool mode) 
{
  writeCmd(mode ? ST7789_DISPON : ST7789_DISPOFF);
}

// ----------------------------------------------------------
void Pico_ST7789::idleDisplay(bool mode) 
{
  writeCmd(mode ? ST7789_IDMON : ST7789_IDMOFF);
}

// ----------------------------------------------------------
void Pico_ST7789::resetDisplay() 
{
  writeCmd(ST7789_SWRESET);
  sleep_ms(5);
}

// ----------------------------------------------------------
void Pico_ST7789::setScrollArea(uint16_t tfa, uint16_t bfa) 
{
  uint16_t vsa = 320-tfa-bfa; // ST7789 320x240 VRAM
  writeCmd(ST7789_VSCRDEF);
  writeData16(tfa);
  writeData16(vsa);
  writeData16(bfa);
}

// ----------------------------------------------------------
void Pico_ST7789::setScroll(uint16_t vsp) 
{
  writeCmd(ST7789_VSCRSADD);
  writeData16(vsp);
}

// ----------------------------------------------------------
void Pico_ST7789::setPartArea(uint16_t sr, uint16_t er) 
{
  writeCmd(ST7789_PTLAR);
  writeData16(sr);
  writeData16(er);
}

// ----------------------------------------------------------
// doesn't work
void Pico_ST7789::setBrightness(uint8_t br) 
{
  //writeCmd(ST7789_WRCACE);
  //writeData(0xb1);  // 80,90,b0, or 00,01,02,03
  //writeCmd(ST7789_WRCABCMB);
  //writeData(120);

  //BCTRL=0x20, dd=0x08, bl=0x04
  int val = 0x04;
  writeCmd(ST7789_WRCTRLD);
  writeData(val);
  writeCmd(ST7789_WRDISBV);
  writeData(br);
}

// ----------------------------------------------------------
// 0 - off
// 1 - idle
// 2 - normal
// 4 - display off
void Pico_ST7789::powerSave(uint8_t mode) 
{
  if(mode==0) {
    writeCmd(ST7789_POWSAVE);
    writeData(0xec|3);
    writeCmd(ST7789_DLPOFFSAVE);
    writeData(0xff);
    return;
  }
  int is = (mode&1) ? 0 : 1;
  int ns = (mode&2) ? 0 : 2;
  writeCmd(ST7789_POWSAVE);
  writeData(0xec|ns|is);
  if(mode&4) {
    writeCmd(ST7789_DLPOFFSAVE);
    writeData(0xfe);
  }
}

// ------------------------------------------------
// Input a value 0 to 511 (85*6) to get a color value.
// The colours are a transition R - Y - G - C - B - M - R.
void Pico_ST7789::rgbWheel(int idx, uint8_t *_r, uint8_t *_g, uint8_t *_b)
{
  idx &= 0x1ff;
  if(idx < 85) { // R->Y  
    *_r = 255; *_g = idx * 3; *_b = 0;
    return;
  } else if(idx < 85*2) { // Y->G
    idx -= 85*1;
    *_r = 255 - idx * 3; *_g = 255; *_b = 0;
    return;
  } else if(idx < 85*3) { // G->C
    idx -= 85*2;
    *_r = 0; *_g = 255; *_b = idx * 3;
    return;  
  } else if(idx < 85*4) { // C->B
    idx -= 85*3;
    *_r = 0; *_g = 255 - idx * 3; *_b = 255;
    return;    
  } else if(idx < 85*5) { // B->M
    idx -= 85*4;
    *_r = idx * 3; *_g = 0; *_b = 255;
    return;    
  } else { // M->R
    idx -= 85*5;
    *_r = 255; *_g = 0; *_b = 255 - idx * 3;
   return;
  }
} 

uint16_t Pico_ST7789::rgbWheel(int idx)
{
  uint8_t r,g,b;
  rgbWheel(idx, &r,&g,&b);
  return RGBto565(r,g,b);
}

// ------------------------------------------------
void Pico_ST7789::drawCircle(int16_t x0, int16_t y0, int16_t r, uint16_t color) {
  int16_t f, ddF_x, ddF_y, x, y;
  f = 1 - r, ddF_x = 1, ddF_y = -2 * r, x = 0, y = r;
  drawPixel(x0  , y0 + r, color);
  drawPixel(x0  , y0 - r, color);
  drawPixel(x0+r, y0    , color);
  drawPixel(x0-r, y0    , color);
  while (x < y) {
    if (f >= 0) {
      y--;
      ddF_y += 2;
      f += ddF_y;
    }
    x++;
    ddF_x += 2;
    f += ddF_x;
    drawPixel(x0 + x, y0 + y, color);
    drawPixel(x0 - x, y0 + y, color);
    drawPixel(x0 + x, y0 - y, color);
    drawPixel(x0 - x, y0 - y, color);
    drawPixel(x0 + y, y0 + x, color);
    drawPixel(x0 - y, y0 + x, color);
    drawPixel(x0 + y, y0 - x, color);
    drawPixel(x0 - y, y0 - x, color);
  }
}

void Pico_ST7789::drawRectWH(uint8_t x, uint8_t y, uint8_t w, uint8_t h, uint16_t color){
  drawFastHLine(x, y, w, color);
  drawFastHLine(x, y + h - 1, w, color);
  drawFastVLine(x, y, h, color);
  drawFastVLine(x + w - 1, y, h, color);
}



void Pico_ST7789::setTextWrap(bool w){
  _wrap = w;
}


// Draw a single text character to screen
void Pico_ST7789::drawChar(uint16_t x, uint16_t y, uint8_t c, uint16_t color, uint16_t bg,  uint8_t size){
  int8_t i, j;
  if((x >= _width) || (y >= _height))
    return;
  if(size < 1) size = 1;
  if((c < ' ') || (c > '~'))
    c = '?';
  for(i=0; i<5; i++ ) {
    uint8_t line;
    line = Font[(c - LCD_ASCII_OFFSET)*5 + i];
    for(j=0; j<7; j++, line >>= 1) {
      if(line & 0x01) {
        if(size == 1) drawPixel(x+i, y+j, color);
        else          fillRect(x+(i*size), y+(j*size), size, size, color);
      }
      else if(bg != color) {
           if(size == 1) drawPixel(x+i, y+j, bg);
           else          fillRect(x+i*size, y+j*size, size, size, bg);
        }
    }
  }
}

// Draw text character array to screen
void Pico_ST7789::drawText(uint16_t x, uint16_t y, const char *_text, uint16_t color, uint16_t bg, uint8_t size) {
  uint16_t cursor_x, cursor_y;
  uint16_t textsize, i;
  cursor_x = x, cursor_y = y;
  textsize = strlen(_text);
  for(i = 0; i < textsize; i++){
    if(_wrap && ((cursor_x + size * 5) > _width)) {
      cursor_x = 0;
      cursor_y = cursor_y + size * 7 + 3 ;
      if(cursor_y > _height) cursor_y = _height;
      if(_text[i] == LCD_ASCII_OFFSET) {
        continue;
      }
    }
    drawChar(cursor_x, cursor_y, _text[i], color, bg, size);
    cursor_x = cursor_x + size * 6;
    if(cursor_x > _width) {
      cursor_x = _width;
    }
  }
}


void Pico_ST7789::setFont(const GFXfont *f) {
  _gfxFont = (GFXfont *)f;
}


// Draw text character array to screen
void Pico_ST7789::drawTextG(uint16_t x, uint16_t y, const char *_text,
			    uint16_t color, uint16_t bg, uint8_t size) {
  uint16_t cursor_x, cursor_y, first_char, last_char;
  uint16_t textlen, i;

  cursor_x = x, cursor_y = y;
  first_char = _gfxFont->first;
  last_char  = _gfxFont->last;
  textlen    = strlen(_text);

  if (bg != color)
    {
      // This will not work with wwapping text but for non wrapping text
      // it is fast and gives a nice continuous background on which is
      // written the text.
      uint16_t tw = 0;
      for ( uint16_t i =0; i < textlen; i++ )
	{
	  tw += size *_gfxFont->glyph[_text[i]-first_char].xAdvance;
	}
      fillRect( cursor_x,
		cursor_y - size * (_gfxFont->yAdvance - 1),
		tw,
		size*_gfxFont->yAdvance,
		bg );
    }

  for(i = 0; i < textlen; i++){
    uint8_t c = _text[i];
    if (c<first_char || c>last_char) {
      continue;
    }

    GFXglyph *glyph = &(_gfxFont->glyph[c-first_char]);
    uint8_t w = glyph->width, h = glyph->height;

    // If we do not do this here we don't catch the whitespace...
    int16_t xo = glyph->xOffset;    
    if(_wrap && ((cursor_x + size * (xo + w)) > _width)) {
      cursor_x = 0;
      cursor_y += (int16_t)size * _gfxFont->yAdvance;
    }
    if((w > 0) && (h > 0)) { // bitmap available
      drawCharG(cursor_x,cursor_y,c,color,color,size);
    }
    cursor_x += glyph->xAdvance * (int16_t)size;
  }
}

void Pico_ST7789::drawCharG(uint16_t x, uint16_t y, uint8_t c, uint16_t color,
			    uint16_t bg,  uint8_t size) {
  c -= (uint8_t) (_gfxFont->first);
  GFXglyph *glyph = _gfxFont->glyph + c;
  uint8_t *bitmap = _gfxFont->bitmap;

  uint16_t bo = glyph->bitmapOffset;
  uint8_t w   = glyph->width, h = glyph->height;
  int8_t xo   = glyph->xOffset, yo = glyph->yOffset;
  uint8_t xx, yy, bits = 0, abit = 0;
  int16_t xo16 = 0, yo16 = 0;

  if (size > 1) {
    xo16 = xo;
    yo16 = yo;
  }

  if (bg != color)
    {
//      fillRect( x-size*glyph->xOffset,
//		y+size*glyph->yOffset,
      fillRect( x,
		y - size*glyph->height,
		size*glyph->xAdvance,
		size*_gfxFont->yAdvance,
		bg);
      
//      fillRect( x,
//		y + size*glyph->height,
//		size*glyph->xAdvance,
//		size*_gfxFont->yAdvance,
//		bg);
    }

  for (yy = 0; yy < h; yy++) {
    for (xx = 0; xx < w; xx++) {
      if (!(abit++ & 7)) {
        bits = bitmap[bo++];
      }
      if (bits & 0x80) {
        if (size == 1) {
          drawPixel(x+xo+xx, y+yo+yy,color);
        } else {
          fillRect(x+(xo16+xx)*size,y+(yo16+yy)*size,size,size,color);
        }
      }
      bits <<= 1;
    }
  }
}
