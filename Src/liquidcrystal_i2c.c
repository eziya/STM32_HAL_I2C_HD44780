#include "liquidcrystal_i2c.h"

extern I2C_HandleTypeDef hi2c1;

uint8_t dpFunction;
uint8_t dpControl;
uint8_t dpMode;
uint8_t dpRows;
uint8_t dpBacklight;

static void Send(uint8_t, uint8_t);
static void Write4Bits(uint8_t);
static void ExpanderWrite(uint8_t);
static void PulseEnable(uint8_t);
static void DelayUS(uint32_t us);

uint8_t special1[8] = {
        0b00000,
        0b11001,
        0b11011,
        0b00110,
        0b01100,
        0b11011,
        0b10011,
        0b00000
};

uint8_t special2[8] = {
        0b11000,
        0b11000,
        0b00110,
        0b01001,
        0b01000,
        0b01001,
        0b00110,
        0b00000
};

void HD44780_Init(uint8_t rows)
{
  dpRows = rows;

  dpBacklight = LCD_BACKLIGHT;

  dpFunction = LCD_4BITMODE | LCD_1LINE | LCD_5x8DOTS;

  if (dpRows > 1)
  {
    dpFunction |= LCD_2LINE;
  }
  else
  {
    dpFunction |= LCD_5x10DOTS;
  }

  /* Wait for initialization */
  HAL_Delay(50);

  ExpanderWrite(dpBacklight);
  HAL_Delay(1000);

  /* 4bit Mode */
  Write4Bits(0x03 << 4);
  DelayUS(4500);

  Write4Bits(0x03 << 4);
  DelayUS(4500);

  Write4Bits(0x03 << 4);
  DelayUS(150);

  Write4Bits(0x02 << 4);

  /* Display Control */
  Send(LCD_FUNCTIONSET | dpFunction, 0);

  dpControl = LCD_DISPLAYON | LCD_CURSOROFF | LCD_BLINKOFF;
  HD44780_Display();
  HD44780_Clear();

  /* Display Mode */
  dpMode = LCD_ENTRYLEFT | LCD_ENTRYSHIFTDECREMENT;
  Send(LCD_ENTRYMODESET | dpMode, 0);

  HD44780_CreateSpecialChar(0, special1);
  HD44780_CreateSpecialChar(1, special2);

  HD44780_Home();
}

void HD44780_Clear()
{
  Send(LCD_CLEARDISPLAY, 0);
  DelayUS(2000);
}

void HD44780_Home()
{
  Send(LCD_RETURNHOME, 0);
  DelayUS(2000);
}

void HD44780_SetCursor(uint8_t col, uint8_t row)
{
  int row_offsets[] = { 0x00, 0x40, 0x14, 0x54 };
  if (row >= dpRows)
  {
    row = dpRows-1;
  }
  Send(LCD_SETDDRAMADDR | (col + row_offsets[row]), 0);
}

void HD44780_NoDisplay()
{
  dpControl &= ~LCD_DISPLAYON;
  Send(LCD_DISPLAYCONTROL | dpControl, 0);
}

void HD44780_Display()
{
  dpControl |= LCD_DISPLAYON;
  Send(LCD_DISPLAYCONTROL | dpControl, 0);
}

void HD44780_NoCursor()
{
  dpControl &= ~LCD_CURSORON;
  Send(LCD_DISPLAYCONTROL | dpControl, 0);
}

void HD44780_Cursor()
{
  dpControl |= LCD_CURSORON;
  Send(LCD_DISPLAYCONTROL | dpControl, 0);
}

void HD44780_NoBlink()
{
  dpControl &= ~LCD_BLINKON;
  Send(LCD_DISPLAYCONTROL | dpControl, 0);
}

void HD44780_Blink()
{
  dpControl |= LCD_BLINKON;
  Send(LCD_DISPLAYCONTROL | dpControl, 0);
}

void HD44780_ScrollDisplayLeft(void)
{
  Send(LCD_CURSORSHIFT | LCD_DISPLAYMOVE | LCD_MOVELEFT, 0);
}

void HD44780_ScrollDisplayRight(void)
{
  Send(LCD_CURSORSHIFT | LCD_DISPLAYMOVE | LCD_MOVERIGHT, 0);
}

void HD44780_LeftToRight(void)
{
  dpMode |= LCD_ENTRYLEFT;
  Send(LCD_ENTRYMODESET | dpMode, 0);
}

void HD44780_RightToLeft(void)
{
  dpMode &= ~LCD_ENTRYLEFT;
  Send(LCD_ENTRYMODESET | dpMode, 0);
}

void HD44780_AutoScroll(void)
{
  dpMode |= LCD_ENTRYSHIFTINCREMENT;
  Send(LCD_ENTRYMODESET | dpMode, 0);
}

void HD44780_NoAutoScroll(void)
{
  dpMode &= ~LCD_ENTRYSHIFTINCREMENT;
  Send(LCD_ENTRYMODESET | dpMode, 0);
}

void HD44780_CreateSpecialChar(uint8_t location, uint8_t charmap[])
{
  location &= 0x7;
  Send(LCD_SETCGRAMADDR | (location << 3), 0);
  for (int i=0; i<8; i++)
  {
    Send(charmap[i], RS);
  }
}

void HD44780_PrintSpecialChar(uint8_t index)
{
  Send(index, RS);
}

void HD44780_LoadCustomCharacter(uint8_t char_num, uint8_t *rows)
{
  HD44780_CreateSpecialChar(char_num, rows);
}

void HD44780_PrintStr(const char c[])
{
  while(*c) Send(*c++, RS);
}

void HD44780_SetBacklight(uint8_t new_val)
{
  if(new_val) HD44780_Backlight();
  else HD44780_NoBacklight();
}

void HD44780_NoBacklight(void)
{
  dpBacklight=LCD_NOBACKLIGHT;
  ExpanderWrite(0);
}

void HD44780_Backlight(void)
{
  dpBacklight=LCD_BACKLIGHT;
  ExpanderWrite(0);
}

static void Send(uint8_t value, uint8_t mode)
{
  uint8_t highnib = value & 0xF0;
  uint8_t lownib = (value<<4) & 0xF0;
  Write4Bits((highnib)|mode);
  Write4Bits((lownib)|mode);
}

static void Write4Bits(uint8_t value)
{
  ExpanderWrite(value);
  PulseEnable(value);
}

static void ExpanderWrite(uint8_t _data)
{
  uint8_t data = _data | dpBacklight;
  HAL_I2C_Master_Transmit(&hi2c1, DEVICE_ADDR, (uint8_t*)&data, 1, HAL_MAX_DELAY);
}

static void PulseEnable(uint8_t _data)
{
  ExpanderWrite(_data | ENABLE);
  DelayUS(1);

  ExpanderWrite(_data & ~ENABLE);
  DelayUS(50);
}

static void DelayUS(uint32_t us)
{
  volatile uint32_t cycles = (SystemCoreClock / 1000000L) * us;
  volatile uint32_t start = DWT->CYCCNT;
  do {
  }while(DWT->CYCCNT - start < cycles);
}
