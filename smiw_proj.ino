#include <SoftwareSerial.h>
#include <Wire.h>

#include "Lcd.h"
#include "TinyGPS.h"
#include "HMC5883L.h"

TinyGPS gps;
SoftwareSerial ss(4, 3);

void setup(void)
{
	Serial.begin(9600);
	pinMode(A0, INPUT);

	LcdInitialise();
	LcdClear();
	LcdString("No elo!");

	ss.begin(9600);
}

int buttonState = LOW;
int previousButtonState = LOW;

void loop()
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
}