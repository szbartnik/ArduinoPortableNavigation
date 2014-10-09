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

void LcdCharacter(char);
void LcdClear(void);
void LcdInitialise(void);
void LcdString(char *);
void LcdWrite(byte, byte);

#endif

