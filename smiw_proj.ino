#include <SoftwareSerial.h>
#include <Wire.h>
#include <SD.h>

#include "InitScreen.h"

// Libraries
#include "Lcd.h"
#include "TinyGPS.h"
#include "HMC5883L.h"

Sd2Card card;
SdVolume volume;
SdFile root;

const int chipSelect = 10;

//TinyGPS gps;
SoftwareSerial ss(4, 3);

void setup(void)
{
	Serial.begin(9600);
	pinMode(A0, INPUT);

	LcdInitialise();
	LcdClear();
	LcdImage(initImg, 0, 0, 84, 6);

	ss.begin(9600);

	SdCardCheck();
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
			SdCardCheck();
			LcdString("No elo!");
		}
	}
}

void SdCardCheck()
{
	Serial.print("\nInitializing SD card...");
	pinMode(10, OUTPUT);

	if (!card.init(SPI_HALF_SPEED, chipSelect)) {
		Serial.println("initialization failed. Things to check:");
		Serial.println("* is a card is inserted?");
		Serial.println("* Is your wiring correct?");
		Serial.println("* did you change the chipSelect pin to match your shield or module?");
		return;
	}
	else {
		Serial.println("Wiring is correct and a card is present.");
	}
}