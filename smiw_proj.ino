#include "Lcd.h"

void setup(void)
{
	Serial.begin(9600);
	pinMode(A0, INPUT);

	LcdInitialise();
	LcdClear();
	LcdString("No elo!");
}

int buttonState = LOW;
int previousButtonState = LOW;

void loop(void)
{
	previousButtonState = buttonState;
	buttonState = digitalRead(A0);

	if (previousButtonState == LOW)
	{
		if (buttonState == HIGH){
			Serial.println("nacisnieto");
			LcdString("No elo!");
		}
	}

	delay(1);
}