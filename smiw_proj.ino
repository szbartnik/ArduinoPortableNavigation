#include <SoftwareSerial.h>
#include <Wire.h>
#include <SD.h>

#include "InitScreen.h"

// Libraries
#include "Lcd.h"
#include "TinyGPS.h"
#include "HMC5883L.h"

HMC5883L compass;
TinyGPS gps;

Sd2Card card;
SdVolume volume;
SdFile root;

const int chipSelect = 10;

SoftwareSerial ss(4, 3);

void setup(void)
{
	pinMode(A0, INPUT);
	pinMode(A1, INPUT);
	pinMode(A2, INPUT);

	pinMode(chipSelect, OUTPUT);

	pinMode(4, INPUT);
	pinMode(3, OUTPUT);

	//Serial.begin(9600);
	ss.begin(9600);

	LcdInitialise();
	LcdClear();
	LcdImage(initImg, 0, 0, 84, 6);

	compass.SetMeasurementMode(Measurement_Continuous);
	compass.SetScale(0.88);
}

int buttonState_0 = LOW;
int buttonState_1 = LOW;
int buttonState_2 = LOW;

int previousButtonState_0 = LOW;
int previousButtonState_1 = LOW;
int previousButtonState_2 = LOW;

char xBuff[16];
char yBuff[16];
char zBuff[16];

String xStr, yStr, zStr;

void loop()
{
	previousButtonState_0 = buttonState_0;
	previousButtonState_1 = buttonState_1;
	previousButtonState_2 = buttonState_2;

	buttonState_0 = digitalRead(A0);
	buttonState_1 = digitalRead(A1);
	buttonState_2 = digitalRead(A2);

	if (previousButtonState_0 == LOW)
	{
		if (buttonState_0 == HIGH)
		{
			LcdClear();
			LcdString("Button 0! ");
			//Serial.println("Nacisnieto 0");

			while (ss.available())
			{
				int c = ss.read();
				if (gps.encode(c))
				{
					char buff[5];
					String str = String(gps.speed());
					str.toCharArray(buff, 5);

					LcdString(buff);
				}
			}
		}
	}

	if (previousButtonState_1 == LOW)
	{
		if (buttonState_1 == HIGH)
		{
			LcdClear();
			LcdString("Button 1! ");
			//Serial.println("Nacisnieto 1");
			SdCardCheck();
		}
	}

	if (previousButtonState_2 == LOW)
	{
		if (buttonState_2 == HIGH)
		{
			LcdClear();
			LcdString("Button 2! ");
			//Serial.println("Nacisnieto 2");
			ReadMagnetometer();
		}
	}
}

void ReadMagnetometer()
{
	delay(200);

	MagnetometerRaw scaledValue = compass.ReadRawAxis();

	delay(200);

	xStr = String(scaledValue.XAxis);
	xStr.toCharArray(xBuff, 16);

	yStr = String(scaledValue.YAxis);
	yStr.toCharArray(yBuff, 16);

	zStr = String(scaledValue.ZAxis);
	zStr.toCharArray(zBuff, 16);

	LcdGoToXY(0, 1);
	LcdString("X: ");
	LcdString(xBuff);
	LcdGoToXY(0, 2);
	LcdString("Y: ");
	LcdString(yBuff);
	LcdGoToXY(0, 3);
	LcdString("Z: ");
	LcdString(zBuff);
	
	/*Serial.print("X: ");
	Serial.print(scaledValue.XAxis);
	Serial.print(", Y: ");
	Serial.print(scaledValue.YAxis);
	Serial.print(", Z: ");
	Serial.print(scaledValue.ZAxis);
	Serial.println("");*/
}

void SdCardCheck()
{
	delay(200);
	LcdString("Init SD... ");
	//Serial.print("\nInitializing SD card...");

	if (!card.init(SPI_HALF_SPEED, chipSelect)) 
	{
		delay(200);
		LcdString("SD card failure! ");
		//Serial.println("SD card failure! ");
	}
	else 
	{
		delay(200);
		LcdString("SD card ok! ");
		//Serial.println("SD card ok! ");
	}
}