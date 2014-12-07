#include "Lcd.h"

void LcdCharacter(char character)
{
	LcdWrite(LCD_D, 0x00);
	for (int index = 0; index < 5; index++)
	{
		LcdWrite(LCD_D, ASCII[character - 0x20][index]);
	}
	LcdWrite(LCD_D, 0x00);
}

void LcdClear(void)
{
	for (int index = 0; index < LCD_X * LCD_Y / 8; index++)
	{
		LcdWrite(LCD_D, 0x00);
	}
}

void LcdInitialise(void)
{
	pinMode(PIN_SCE, OUTPUT);
	pinMode(PIN_RESET, OUTPUT);
	pinMode(PIN_DC, OUTPUT);
	pinMode(PIN_SDIN, OUTPUT);
	pinMode(PIN_SCLK, OUTPUT);
	digitalWrite(PIN_RESET, LOW);
	digitalWrite(PIN_RESET, HIGH);
	LcdWrite(LCD_C, 0x21);  // LCD Extended Commands.
	LcdWrite(LCD_C, 0xB8);  // Set LCD Vop (Contrast). 
	LcdWrite(LCD_C, 0x04);  // Set Temp coefficent. //0x04
	LcdWrite(LCD_C, 0x14);  // LCD bias mode 1:48. //0x13
	LcdWrite(LCD_C, 0x0C);  // LCD in normal mode.
	LcdWrite(LCD_C, 0x20);
	LcdWrite(LCD_C, 0x0C);
}

void LcdString(char *characters)
{
	while (*characters)
	{
		LcdCharacter(*characters++);
	}
}

void LcdWrite(byte dc, byte data)
{
	digitalWrite(PIN_DC, dc);
	digitalWrite(PIN_SCE, LOW);
	shiftOut(PIN_SDIN, PIN_SCLK, MSBFIRST, data);
	digitalWrite(PIN_SCE, HIGH);
}

void LcdImage(unsigned char img[], char x0, char y0, char w, char h)
{
	unsigned int i, j, k;

	for (i = 0, j = 0, k = 0; i<h; i++)
	{
		/* Komenda  LCD "Set Y address of RAM" */
		LcdWrite(LCD_C, 0x40 | (i + y0));
		/* Komenda "Set X address of RAM"*/
		LcdWrite(LCD_C, 0x80 | (x0));

		/* Kopiowanie z FLASH do pamiêci obrazu LCD  */
		for (j = 0; j<w; j++, k++)
			LcdWrite(LCD_D, pgm_read_byte(&img[k]));
	}
}