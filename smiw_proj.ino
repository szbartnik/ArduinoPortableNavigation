#define GPS_ONs

#include <Wire.h>
#include <SD.h>

#include "InitScreen.h"

// Libraries
#include "Lcd.h"

#include "HMC5883L.h"

#ifdef GPS_ON
#include "TinyGPS.h"
#endif

HMC5883L compass;

#ifdef GPS_ON
TinyGPS gps;
#endif

Sd2Card card;
SdVolume volume;
SdFile root;

const int chipSelect = 10;

void setup(void)
{
	pinMode(A0, INPUT);
	pinMode(A1, INPUT);
	pinMode(A2, INPUT);
	pinMode(A3, INPUT);

	pinMode(chipSelect, OUTPUT);


	LcdInitialise();
	LcdClear();
	LcdImage(initImg, 0, 0, 84, 6);

	compass.SetMeasurementMode(Measurement_Continuous);
	compass.SetScale(0.88);

	Serial.begin(9600);
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
			GpsTest();
		}
	}

	if (previousButtonState_1 == LOW)
	{
		if (buttonState_1 == HIGH)
		{
			LcdClear();
			LcdString("Button 1! ");
			SdCardCheck();
		}
	}

	if (previousButtonState_2 == LOW)
	{
		if (buttonState_2 == HIGH)
		{
			LcdClear();
			LcdString("Button 2! ");
			ReadMagnetometer();
		}
	}
}

void GpsTest()
{
#ifdef GPS_ON
	smartdelay(1000);

	char buff[5];
	String str = String(gps.speed());
	str.toCharArray(buff, 5);

	LcdString(buff);
#endif
}

#ifdef GPS_ON
static void smartdelay(unsigned long ms)
{
	unsigned long start = millis();
	do
	{
		while (Serial.available())
		{
			LcdString("_xx_");
			gps.encode(Serial.read());
		}
	} while (millis() - start < ms);
}
#endif

void ReadMagnetometer()
{
	MagnetometerRaw scaledValue = compass.ReadRawAxis();

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
}

void SdCardCheck()
{
	LcdString("Init SD... ");

	if (!card.init(SPI_HALF_SPEED, chipSelect)) 
	{
		LcdString("SD card failure! ");
	}
	else 
	{
		LcdString("SD card ok! ");
	}
}