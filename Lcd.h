#ifndef _LCD_h
#define _LCD_h

#include "Arduino.h"
#include "CharDefs.h"

// ##### DEFINES ##### //

#define PIN_SCE   9
#define PIN_RESET 8
#define PIN_DC    7
#define PIN_SDIN  6
#define PIN_SCLK  5

#define LCD_C     LOW
#define LCD_D     HIGH

#define LCD_X     84
#define LCD_Y     48

#define LCD_SETX  0x80   
#define LCD_SETY  0x40 

#define LCD_HOME LcdWrite(LCD_C, 0x40);LcdWrite(LCD_C, 0x80);
#define LCD_GOTO(x,y) LcdWrite(LCD_C, 0x40|(y));LcdWrite(LCD_C, 0x80|(x));

// ##### Functions declarations ##### //

void LcdGoToXY(int, int);
void LcdCharacter(char character);
void LcdClear(void);
void LcdInitialise(void);

void LcdString(char *characters, bool inversed = false);
void LcdString(const __FlashStringHelper* pData, bool inversed = false);

void LcdWrite(byte dc, byte data);
void LcdImage(unsigned char img[], char x0, char y0, char w, char h);

#endif

