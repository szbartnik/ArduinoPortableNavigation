#include <Arduino.h>

#include <SoftwareSerial\SoftwareSerial.h>
#include <Wire.h>

#include "InitScreen.h"

#define PGMT( pgm_ptr ) ( reinterpret_cast< const __FlashStringHelper * >( pgm_ptr ) )
#define LOC_MENU  -1
#define LOC_SDREC  0
#define LOC_NAV    1
#define LOC_GPSPOS 2
#define LOC_MAG    3

// Libraries
#include <SD.h>
#include "Lcd.h"
#include "HMC5883L.h"
#include "TinyGPS.h"

HMC5883L compass;
TinyGPS gps;
Sd2Card card;

static const byte chipSelect = 10;

SoftwareSerial gpsSerial(4, 3);

static const byte DOWN_KEY = A3;
static const byte UP_KEY = A2;
static const byte EXECUTE_KEY = A1;
static const byte EXIT_KEY = A0;

byte DOWN_buttonState = LOW;
byte UP_buttonState = LOW;
byte EXECUTE_buttonState = LOW;
byte EXIT_buttonState = LOW;

byte DOWN_previousButtonState = LOW;
byte UP_previousButtonState = LOW;
byte EXECUTE_previousButtonState = LOW;
byte EXIT_previousButtonState = LOW;

char currentView;
char markedMenuOption;

void setup(void)
{
	gpsSerial.begin(9600);
	Serial.begin(9600);

	pinMode(DOWN_KEY, INPUT);
	pinMode(UP_KEY, INPUT);
	pinMode(EXECUTE_KEY, INPUT);
	pinMode(EXIT_KEY, INPUT);

	pinMode(chipSelect, OUTPUT);

	compass.SetMeasurementMode(Measurement_Continuous);
	compass.SetScale(0.88);

	LcdInitialise();
	LcdClear();
	LcdImage(initImg, 0, 0, 84, 6);

	delay(1000);

	currentView = -1;
	markedMenuOption = 0;
	refreshView();
}

static void refreshView()
{
	// If we are going to show main menu
	if (currentView == LOC_MENU)
	{
		LcdGoToXY(0, 0);
		LcdString(F("-----Menu-----"));
		LcdString(F("              "));
		LcdString(F("Nagrywanie SD "), markedMenuOption == LOC_SDREC);
		LcdString(F("Nav. do punktu"), markedMenuOption == LOC_NAV);
		LcdString(F("Pozycja GPS   "), markedMenuOption == LOC_GPSPOS);
		LcdString(F("Kompas        "), markedMenuOption == LOC_MAG);
	}
	// If we are going to show specific viewy
	else
	{

	}
}

static int freeRam()
{
	extern int __heap_start, *__brkval;
	int v;
	return (int)&v - (__brkval == 0 ? (int)&__heap_start : (int)__brkval);
}

static void smartdelay(unsigned long ms)
{
	unsigned long start = millis();
	do
	{
		while (gpsSerial.available())
		{
			gps.encode(gpsSerial.read());
		}
	} while (millis() - start < ms);
}

static void print_float(float val, float invalid, int len, int prec)
{
	if (val == invalid)
	{
		while (len-- > 1)
			Serial.print('*');
		Serial.print(' ');
	}
	else
	{
		Serial.print(val, prec);
		int vi = abs((int)val);
		int flen = prec + (val < 0.0 ? 2 : 1); // . and -
		flen += vi >= 1000 ? 4 : vi >= 100 ? 3 : vi >= 10 ? 2 : 1;
		for (int i = flen; i<len; ++i)
			Serial.print(' ');
	}
	smartdelay(0);
}

static void print_int(unsigned long val, unsigned long invalid, int len)
{
	char sz[10];
	if (val == invalid)
		strcpy(sz, "*******");
	else
		sprintf(sz, "%ld", val);
	sz[len] = 0;
	for (int i = strlen(sz); i<len; ++i)
		sz[i] = ' ';
	if (len > 0)
		sz[len - 1] = ' ';
	Serial.print(sz);
	smartdelay(0);
}

static void print_str(const char *str, int len)
{
	int slen = strlen(str);
	for (int i = 0; i<len; ++i)
		Serial.print(i<slen ? str[i] : ' ');
	smartdelay(0);
}

static void moveMenu(bool isDirectionUp)
{
	markedMenuOption = isDirectionUp 
		? (markedMenuOption == 0 
			? 3 
			: markedMenuOption - 1) 
		: (markedMenuOption == 3 
			? 0 
			: markedMenuOption + 1);
}

static void GpsTest()
{
	smartdelay(1000);

	float flat, flon;
	static const double LONDON_LAT = 51.508131, LONDON_LON = -0.128002;

	print_int(gps.satellites(), TinyGPS::GPS_INVALID_SATELLITES, 5);
	gps.f_get_position(&flat, &flon);
	print_float(flat, TinyGPS::GPS_INVALID_F_ANGLE, 10, 6);
	print_float(flon, TinyGPS::GPS_INVALID_F_ANGLE, 11, 6);
	print_float(gps.f_altitude(), TinyGPS::GPS_INVALID_F_ALTITUDE, 7, 2);
	print_float(gps.f_course(), TinyGPS::GPS_INVALID_F_ANGLE, 7, 2);
	print_float(gps.f_speed_kmph(), TinyGPS::GPS_INVALID_F_SPEED, 6, 2);
	print_int(flat == TinyGPS::GPS_INVALID_F_ANGLE ? 0xFFFFFFFF : (unsigned long)TinyGPS::distance_between(flat, flon, LONDON_LAT, LONDON_LON) / 1000, 0xFFFFFFFF, 9);
	Serial.println();
}

static void ReadMagnetometer()
{
	char buff[6];

	MagnetometerRaw scaledValue = compass.ReadRawAxis();

	String xStr = String(scaledValue.XAxis);
	xStr.toCharArray(buff, 6);
	LcdGoToXY(0, 1);
	LcdString("X: ");
	LcdString(buff);
	Serial.write("X: ");
	Serial.write(buff);

	String yStr = String(scaledValue.YAxis);
	yStr.toCharArray(buff, 6);
	LcdGoToXY(0, 2);
	LcdString("Y: ");
	LcdString(buff);
	Serial.write("Y: ");
	Serial.write(buff);

	String zStr = String(scaledValue.ZAxis);
	zStr.toCharArray(buff, 6);
	LcdGoToXY(0, 3);
	LcdString("Z: ");
	LcdString(buff);
	Serial.write("Z: ");
	Serial.write(buff);
}

static void SdCardCheck()
{
	Serial.println(freeRam());

	LcdString(F("Init SD... "));
	Serial.write("Init SD... ");

	if (!card.init(SPI_HALF_SPEED, chipSelect)) 
	{
		LcdString(F("SD card failure! "));
		Serial.write("SD card failure! ");
	}
	else 
	{
		LcdString(F("SD card ok! "));
		Serial.write("SD card ok! ");
	}
}

void loop()
{
	DOWN_previousButtonState = DOWN_buttonState;
	UP_previousButtonState = UP_buttonState;
	EXECUTE_previousButtonState = EXECUTE_buttonState;
	EXIT_previousButtonState = EXIT_buttonState;

	DOWN_buttonState = digitalRead(DOWN_KEY);
	UP_buttonState = digitalRead(UP_KEY);
	EXECUTE_buttonState = digitalRead(EXECUTE_KEY);
	EXIT_buttonState = digitalRead(EXIT_KEY);


	if (DOWN_buttonState == HIGH && DOWN_previousButtonState == LOW)
	{
		ButtonClicked(DOWN_KEY);
	}
	else if (UP_buttonState == HIGH && UP_previousButtonState == LOW)
	{
		ButtonClicked(UP_KEY);
	}
	else if (EXECUTE_buttonState == HIGH && EXECUTE_previousButtonState == LOW)
	{
		ReadMagnetometer();
		ButtonClicked(EXECUTE_KEY);
	}
	else if (EXIT_buttonState == HIGH && EXIT_previousButtonState == LOW)
	{
		SdCardCheck();
		GpsTest();

		ButtonClicked(EXIT_KEY);
	}
}

void ButtonClicked(byte buttonId)
{
	switch (buttonId)
	{
		case DOWN_KEY:
			moveMenu(false);
			break;
		case UP_KEY:
			moveMenu(true);
			break;
		case EXECUTE_KEY:
			if (currentView == LOC_MENU)
				currentView = markedMenuOption;
			break;
		case EXIT_KEY:
			if (currentView != LOC_MENU)
				currentView = LOC_MENU;
			break;
	}

	refreshView();
	delay(150);
}